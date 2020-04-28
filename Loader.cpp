#include "Core.h"
#include "Loader.h"
#include "ModuleList.h"
#include "ModuleConfig.h"
#include "GlobalConsole.h"
#include "xoPrimitiveConsole.h"

#include <QDir>
#include <QApplication>
#include <QPluginLoader>

Loader::Loader(Server *server, Hub *hub, QObject *parent) : QObject(parent), server(server), hub(hub)
{

}

Loader::~Loader()
{
    for(auto process : processesByAppName)
    {
        process->kill();
        process->waitForFinished();
    }
    processesByAppName.clear();
}

void Loader::load()
{
    connect(server, &Server::moduleConnection, this, [=](ModuleProxyONB *module)
    {
        if (!module)
        {
            qDebug() << "Server returned null module";
            return;
        }

        hub->addModule(module);

        connect(module, &ModuleProxyONB::ready, this, [=]() { Core::Instance()->getHub()->checkCurrentSchemeComponents(); }, Qt::QueuedConnection);

        QString configsPath = Core::FolderConfigs + module->name();
        QDir dir(configsPath);

        if(dir.exists())
        {
            moduleByName[module->name()] = module;
            emit configWritten(module);
            return;
        }

        moduleConnectsModuleName[module->name()] = connect(module, &ModuleProxyONB::ready, this, [=]()
        {
            moduleByName[module->name()] = module;

            ConfigManager::writeModuleConfig(module);
            emit configWritten(module);

            disconnect(moduleConnectsModuleName[module->name()]);

            if(startTypeByAppName.contains(module->name()))
                if(startTypeByAppName[module->name()] == COLD)
                    killApplication(module->name());

            moduleConnectsModuleName.remove(module->name());
        }, Qt::QueuedConnection);

    }, Qt::QueuedConnection);

    ModuleList::Instance()->init();
    parseApplicationsStartOptions();

    QDir pluginsDirectory(Core::FolderPlugins);
    for(auto pluginName : pluginsDirectory.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
    {
        pluginList << pluginName;

        PluginManager::Instance()->load(pluginName, getModulePath(pluginName, ModuleConfig::Type::PLUGIN));
    }

    QDir applicationsDirectory(Core::FolderModules);
    for(auto applicationName : applicationsDirectory.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
    {
        applicationList << applicationName;

        appNames.insert(applicationName);

        QString path = getModulePath(applicationName, ModuleConfig::Type::MODULE);

        QFileInfo info(path);
        if(!info.exists()) { qDebug() << "Module doesn't exist" << applicationName; continue; }

        if(startTypeByAppName[applicationName] == HOT) startApplication(applicationName);
    }

    bool uiProvided = false;
    QDir corePluginsDirectory(Core::FolderCorePlugins);
    for(auto corePluginPath : corePluginsDirectory.entryList(QStringList{"*" + Core::PluginExtensionDot}))
    {
        QFileInfo fileInfo(corePluginPath);
        QString pluginName = fileInfo.completeBaseName();
        auto filepath = corePluginsDirectory.absoluteFilePath(corePluginPath);

        auto plugin = QPluginLoader(filepath).instance();
        if (plugin)
        {
            auto corePlugin = qobject_cast<xoCorePlugin*>(plugin);
            if (corePlugin)
            {
                corePlugin->start();
                if(!uiProvided)
                    uiProvided = corePlugin->providesUI();

                QString name = fileInfo.completeBaseName();
                GlobalConsole::writeItem("xoCorePlugin loaded: " + pluginName);
                corePluginsByName[pluginName] = corePlugin;
            }
        }
    }
    if (!uiProvided)
    {
        auto console = new xoPrimitiveConsole();
        console->show();
    }
}

void Loader::startApplication(QString applicationName)
{
    if(!appNames.contains(applicationName)) return;

    if(processesByAppName.contains(applicationName)) return;

    auto process = new QProcess(this);

    auto appPath = getModulePath(applicationName, ModuleConfig::Type::MODULE);

    process->setWorkingDirectory(QFileInfo(appPath).dir().absolutePath());
    process->start(appPath, QStringList() << "-i" << "127.0.0.1" << "-p" << QString::number(server->getPort()));

    processesByAppName[applicationName] = process;

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), process, [=](int, QProcess::ExitStatus)
    {
        processesByAppName.remove(applicationName);

        hub->checkCurrentSchemeComponents();
    });
}

bool Loader::getApplicationStartType(QString applicationName)
{
    return startTypeByAppName.contains(applicationName) ? startTypeByAppName[applicationName] : true;
}

void Loader::killApplication(QString applicationName)
{
    if(!appNames.contains(applicationName)) return;

    if(startTypeByAppName[applicationName] == HOT) return;

    auto process = processesByAppName[applicationName];
    process->kill();
    process->waitForFinished();

    processesByAppName.remove(applicationName);

    moduleByName.remove(applicationName);
}

bool Loader::applicationIsRunning(QString applicationName)
{
    return processesByAppName.contains(applicationName);
}

void Loader::updateModuleStartType(QString moduleName, ModuleStartType type)
{
    startTypeByAppName[moduleName] = type;

    switch(type)
    {
        case HOT: startApplication(moduleName); break;
        case COLD: killApplication(moduleName);  break;
    }

    QFile startOptionsFile(QApplication::applicationDirPath() + "/ModuleStartOptions");

    if(!startOptionsFile.open(QIODevice::WriteOnly)) return;

    QTextStream stream(&startOptionsFile);

    for(auto appName : startTypeByAppName.keys())
    {
        stream << appName << " " << ((startTypeByAppName.value(appName) == ModuleStartType::HOT) ? 1 : 0) << endl;
    }
}

void Loader::parseApplicationsStartOptions()
{
    QFile startOptionsFile(QApplication::applicationDirPath() + "/ModuleStartOptions");
    if(startOptionsFile.open(QIODevice::ReadOnly))
    {
        QString contents = startOptionsFile.readAll();

        QRegExp regExp("([^\\s]+)\\s(\\d+)(\\n)?");
        regExp.setMinimal(true);
        int position = 0;

        while ((position = regExp.indexIn(contents, position)) != -1)
        {
            QString name = regExp.cap(1);
            int value = regExp.cap(2).toInt();

            startTypeByAppName[name] = value > 0 ? ModuleStartType::HOT : ModuleStartType::COLD;
            position += regExp.matchedLength();
        }
    }
}

void Loader::refreshConfigsForModule(QString moduleName)
{
    QString configsPath = Core::FolderConfigs + moduleName;
    QDir configsDir(configsPath);
    configsDir.removeRecursively();

    if(appNames.contains(moduleName)) //является приложением
    {
        if(processesByAppName.contains(moduleName)) //приложение запущено
        {
            ConfigManager::writeModuleConfig(moduleByName[moduleName]);
        }
        else //приложение не запущено
        {
            startApplication(moduleName); //дальше само разберется
        }
    }
    else //является плагином
    {
        ConfigManager::writeModuleConfig(moduleByName[moduleName]);
    }
}

QString Loader::getModulePath(QString moduleName, ModuleConfig::Type type)
{
    QString suffix;
#ifdef QT_DEBUG
    suffix = "d";
#endif

    QString extension;
    QString folder;
    switch (type)
    {
        case ModuleConfig::Type::MODULE:
        {
            folder = Core::FolderModules;
            extension = Core::ApplicationExtensionDot;
            break;
        }
        case ModuleConfig::Type::PLUGIN:
        {
            folder = Core::FolderPlugins;
            extension = Core::PluginExtensionDot;
            break;
        }
    }

    return folder + moduleName + "/" + moduleName + suffix + extension;
}
