#include "Core.h"
#include "Loader.h"
#include "ModuleList.h"

#include <QDir>
#include <QApplication>

Loader::Loader(Server *server, Hub *hub, QObject *parent) : QObject(parent), server(server), hub(hub)
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

        PluginManager::Instance()->load(pluginName, pluginsDirectory.absolutePath() + "/" + pluginName + ".dll");
    }

    QDir applicationsDirectory(Core::FolderModules);
    for(auto applicationName : applicationsDirectory.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
    {
        applicationList << applicationName;

        appNames.insert(applicationName);

        QString path = getApplicationPath(applicationName);

        QFileInfo info(path);
        if(!info.exists()) { qDebug() << "Module doesn't exist" << applicationName; continue; }

        if(startTypeByAppName[applicationName] == HOT) startApplication(applicationName);
    }
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

void Loader::startApplication(QString applicationName)
{
    if(!appNames.contains(applicationName)) return;

    if(processesByAppName.contains(applicationName)) return;

    auto process = new QProcess(this);

    auto appPath = getApplicationPath(applicationName);

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

QString Loader::getApplicationPath(QString applicationName)
{
#ifdef Q_OS_LINUX

#ifdef QT_DEBUG
    return Core::FolderModules + "/" + applicationName + "/" + applicationName + "d";
#else
    return Core::FolderModules + "/" + applicationName + "/" + applicationName;
#endif

#else
#ifdef QT_DEBUG
    return Core::FolderModules + "/" + applicationName + "/" + applicationName + "d.exe";
#else
    return Core::FolderModules + "/" + applicationName + "/" + applicationName + ".exe";
#endif
#endif
}
