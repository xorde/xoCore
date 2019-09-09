#include "Core.h"
#include "ModuleList.h"
#include "ModuleBaseLibONB.h"
#include "GlobalConsole.h"
#include <QProcess>
#include <QPair>
#include "fileutilities.h"
#include <QApplication>
#include <QPluginLoader>

Core *Core::_instance = nullptr;

QString Core::FolderConfigs = "xoConfigs/";
QString Core::FolderSchemes = "xoSchemes/";
QString Core::FolderPlugins = "xoPlugins/";
QString Core::FolderModules = "xoModules/";
QString Core::FolderCorePlugins = "xoCorePlugins/";

QString Core::FileExtensionScheme = "scheme";
QString Core::FileExtensionSchemeDot = "."+FileExtensionScheme;
QString Core::FileExtensionConfig = "config";
QString Core::FileExtensionConfigDot = "."+FileExtensionConfig;

Core::Core(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<ONBPacket>("ONBPacket");

    QString appPath = QCoreApplication::applicationDirPath() + "/";

    FolderConfigs.prepend(appPath);
    FolderSchemes.prepend(appPath);
    FolderPlugins.prepend(appPath);
    FolderModules.prepend(appPath);
    FolderCorePlugins.prepend(appPath);

    _instance = this;
    FileUtilities::createIfNotExists(FolderConfigs);
    FileUtilities::createIfNotExists(FolderSchemes);
    FileUtilities::createIfNotExists(FolderPlugins);
    FileUtilities::createIfNotExists(FolderModules);

    m_server = new Server();
    m_server->startListening();

    connect(m_server, &Server::moduleConnection, this, [=](ModuleProxyONB *module)
    {
        if (!module)
        {
            qDebug() << "Server returned null module";
            return;
        }


        m_hub->addModule(module);

        connect(module, &ModuleProxyONB::ready, this, [=]()
        {
            Core::Instance()->getHub()->checkCurrentSchemeComponents();
        }, Qt::QueuedConnection);

        QString configsPath = FolderConfigs + module->name();
        QDir dir(configsPath);

        if(dir.exists())
        {
            m_moduleByName[module->name()] = module;
            emit configWritten(module);
            return;
        }

        m_moduleConnectsModuleName[module->name()] = connect(module, &ModuleProxyONB::ready, this, [=]()
        {
            m_moduleByName[module->name()] = module;

            ConfigManager::writeModuleConfig(module);
            emit configWritten(module);

            disconnect(m_moduleConnectsModuleName[module->name()]);

            if(m_startTypeByAppName.contains(module->name()))
                if(m_startTypeByAppName[module->name()] == COLD)
                    killApplication(module->name());

            m_moduleConnectsModuleName.remove(module->name());
        }, Qt::QueuedConnection);

    }, Qt::QueuedConnection);

    m_scheme = new Scheme();
    connect(m_scheme, &Scheme::loaded, this, [=]() {
        GlobalConsole::writeLine("Scheme loaded: " + m_scheme->getLastLoadedPath());
    }, Qt::QueuedConnection);

    m_hub = new Hub(this);
    m_hub->setScheme(m_scheme);

    connect(m_hub, &Hub::enableChanged, this, [=](bool enabled) {
        GlobalConsole::writeLine(QString("Scheme ") + (enabled ? "started" : "stopped"));
    }, Qt::QueuedConnection);


    ModuleList::Instance()->init();
    parseApplicationsStartOptions();

    QDir applicationsDirectory(FolderModules);
    for(auto applicationName : applicationsDirectory.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
    {
        m_appNames.insert(applicationName);

        QString path = getApplicationPath(applicationName);

        QFileInfo info(path);
        if(!info.exists()) { qDebug() << "Module doesn't exist" << applicationName; continue; }

        if(m_startTypeByAppName[applicationName] == HOT) startApplication(applicationName);
    }
}

Core::~Core()
{
    for(auto process : m_processesByAppName)
    {
        process->kill();
        process->waitForFinished();
    }
    m_processesByAppName.clear();

    if (m_server) delete m_server;
    if (m_hub) delete m_hub;
    if (m_scheme) delete m_scheme;

    ModuleList::removeList();
}

void Core::startApplication(QString applicationName)
{
    if(!m_appNames.contains(applicationName)) return;

    if(m_processesByAppName.contains(applicationName)) return;

    auto process = new QProcess(this);

    auto appPath = getApplicationPath(applicationName);

    process->setWorkingDirectory(QFileInfo(appPath).dir().absolutePath());
    process->start(appPath, QStringList() << "-i" << "127.0.0.1" << "-p" << QString::number(m_server->getPort()));
    process->waitForStarted();

    m_processesByAppName[applicationName] = process;
}

void Core::killApplication(QString applicationName)
{
    if(!m_appNames.contains(applicationName)) return;

    if(m_startTypeByAppName[applicationName] == HOT) return;

    auto process = m_processesByAppName[applicationName];
    process->kill();
    process->waitForFinished();

    m_processesByAppName.remove(applicationName);

    m_moduleByName.remove(applicationName);
}

bool Core::applicationIsRunning(QString applicationName)
{
    return m_processesByAppName.contains(applicationName);
}

bool Core::getApplicationStartType(QString applicationName)
{
    return m_startTypeByAppName.contains(applicationName) ? m_startTypeByAppName[applicationName] : true;
}

void Core::updateModuleStartType(QString moduleName, ModuleStartType type)
{
    m_startTypeByAppName[moduleName] = type;

    switch(type)
    {
    case HOT: startApplication(moduleName); break;
    case COLD:  killApplication(moduleName);  break;
    }

    QFile startOptionsFile(QApplication::applicationDirPath() + "/ModuleStartOptions");

    if(!startOptionsFile.open(QIODevice::WriteOnly)) return;

    QTextStream stream(&startOptionsFile);

    for(auto appName : m_startTypeByAppName.keys())
    {
        stream << appName << " " << ((m_startTypeByAppName.value(appName) == ModuleStartType::HOT) ? 1 : 0) << endl;
    }
}

ComponentInfo* Core::createComponentInScheme(QString componentType, QString moduleName)
{
    if (!m_scheme) return nullptr;

    //TODO: check for moduleName and componentType availability
    QString type = componentType;
    QString title = type;

    int i = 1;
    QString newTitle;
    do
    {
        newTitle = title + "_" + QString::number(i);
        i++;
    }
    while (m_scheme->containsComponentName(newTitle));
    title = newTitle;

    auto componentInfo = new ComponentInfo();
    componentInfo->name = title;
    componentInfo->type = type;
    componentInfo->parentModule = moduleName;

    m_scheme->addComponent(componentInfo);

    if(getApplicationStartType(moduleName) == COLD)
    {
        m_componentCountByModuleName[moduleName]++;
        startApplication(moduleName);
    }

    return componentInfo;
}

bool Core::removeComponentFromScheme(ComponentInfo *componentInfo)
{
    if(getApplicationStartType(componentInfo->parentModule) == COLD)
    {
        int& counter = m_componentCountByModuleName[componentInfo->parentModule];
        counter--;

        if(counter == 0)
        {
            killApplication(componentInfo->parentModule);
        }
    }

    return m_scheme->removeComponentByName(componentInfo->name);
}

void Core::parseApplicationsStartOptions()
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

            m_startTypeByAppName[name] = value > 0 ? ModuleStartType::HOT : ModuleStartType::COLD;
            position += regExp.matchedLength();
        }
    }
}

bool Core::loadScheme(QString schemePath)
{
    if (m_hub->isEnabled())
    {
        m_hub->setIsEnabled(false);
    }

    bool loaded = m_scheme->load(schemePath);
    m_hub->setScheme(m_scheme);
    return loaded;
}

bool Core::deleteScheme(QString schemePath)
{
    if (!schemePath.endsWith(Core::FileExtensionSchemeDot))
    {
        schemePath += Core::FileExtensionSchemeDot;
    }

    if (!QFile::exists(schemePath) && QFile::exists(FolderSchemes+schemePath))
    {
        schemePath = FolderSchemes+schemePath;
    }

    if (m_scheme && m_scheme->getLastLoadedPath() == schemePath)
    {
        m_hub->setIsEnabled(false);
        m_scheme->clear();
    }

    return QFile::remove(schemePath);
}

void Core::loadCorePlugins()
{
    auto corePluginPaths = FileUtilities::getFilesOfType(FolderCorePlugins, "dll", false, true);

    foreach(QString path, corePluginPaths)
    {
        QFileInfo fileInfo(path);

        QString corePluginName = FileUtilities::filename(path);

        QPluginLoader loader(path);
        QObject* plugin = loader.instance();
        if (plugin)
        {
            xoCorePlugin* corePlugin = qobject_cast<xoCorePlugin*>(plugin);
            if (corePlugin)
            {
                m_corePlugins.insert(corePluginName, corePlugin);
                GlobalConsole::writeItem("xoCorePlugin loaded: " + corePluginName);
            }
        }
    }

}

QMap<QString, xoCorePlugin *> Core::getCorePlugins()
{
    return m_corePlugins;
}

Core *Core::Instance()
{
    return _instance;
}

void Core::refreshConfigsForModule(QString moduleName)
{
    QString configsPath = FolderConfigs + moduleName;
    QDir configsDir(configsPath);
    configsDir.removeRecursively();

    if(m_appNames.contains(moduleName)) //является приложением
    {
        if(m_processesByAppName.contains(moduleName)) //приложение запущено
        {
            ConfigManager::writeModuleConfig(m_moduleByName[moduleName]);
        }
        else //приложение не запущено
        {
            startApplication(moduleName); //дальше само разберется
        }
    }
    else //является плагином
    {
        ConfigManager::writeModuleConfig(m_moduleByName[moduleName]);
    }
}

Server *Core::getServer()
{
    return m_server;
}

Hub *Core::getHub()
{
    return m_hub;
}

Scheme *Core::getScheme()
{
    return m_scheme;
}

QString Core::getApplicationPath(QString applicationName)
{
#ifdef Q_OS_LINUX

#ifdef QT_DEBUG
    return FolderModules + "/" + applicationName + "/" + applicationName + "d";
#else
    return FolderModules + "/" + applicationName + "/" + applicationName;
#endif

#else
#ifdef QT_DEBUG
    return FolderModules + "/" + applicationName + "/" + applicationName + "d.exe";
#else
    return FolderModules + "/" + applicationName + "/" + applicationName + ".exe";
#endif
#endif
}
