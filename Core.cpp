#include "Core.h"
#include "ModuleBaseLibONB.h"
#include "GlobalConsole.h"
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

    m_server = new Server(this);
    m_server->startListening();

    m_scheme = new Scheme(this);
    connect(m_scheme, &Scheme::loaded, this, [=]() { GlobalConsole::writeLine("Scheme loaded: " + m_scheme->getLastLoadedPath()); }, Qt::QueuedConnection);

    m_hub = new Hub(this);
    m_hub->setScheme(m_scheme);

    connect(m_hub, &Hub::enableChanged, this, [=](bool enabled) { GlobalConsole::writeLine(QString("Scheme ") + (enabled ? "started" : "stopped")); }, Qt::QueuedConnection);

    loader = new Loader(m_server, m_hub, this);
}

Core::~Core()
{
    ModuleList::removeList();
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

    if(loader->getApplicationStartType(moduleName) == COLD)
    {
        m_componentCountByModuleName[moduleName]++;
        loader->startApplication(moduleName);
    }

    return componentInfo;
}

bool Core::removeComponentFromScheme(ComponentInfo *componentInfo)
{
    if(loader->getApplicationStartType(componentInfo->parentModule) == COLD)
    {
        int& counter = m_componentCountByModuleName[componentInfo->parentModule];
        counter--;

        if(counter == 0) loader->killApplication(componentInfo->parentModule);
    }

    return m_scheme->removeComponentByName(componentInfo->name);
}

bool Core::loadScheme(QString schemePath)
{
    if (m_hub->isEnabled()) m_hub->setIsEnabled(false);

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
#ifdef Q_OS_UNIX
    auto corePluginPaths = FileUtilities::getFilesOfType(FolderCorePlugins, "so", false, true);
#else
    auto corePluginPaths = FileUtilities::getFilesOfType(FolderCorePlugins, "dll", false, true);
#endif

    for(auto path : corePluginPaths)
    {
        QFileInfo fileInfo(path);

        QString corePluginName = FileUtilities::filename(path);

        auto plugin = QPluginLoader(path).instance();
        if (plugin)
        {
            auto corePlugin = qobject_cast<xoCorePlugin*>(plugin);
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

Server *Core::getServer()
{
    return m_server;
}

Hub *Core::getHub()
{
    return m_hub;
}

Loader *Core::getLoader()
{
    return loader;
}

Scheme *Core::getScheme()
{
    return m_scheme;
}
