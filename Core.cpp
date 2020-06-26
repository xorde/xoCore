#include "Core.h"
#include "ModuleBaseLibONB.h"
#include "GlobalConsole.h"
#include "fileutilities.h"

#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QPluginLoader>
#include <QMessageBox>

QString Core::FolderConfigs = "xoConfigs/";
QString Core::FolderSchemes = "xoSchemes/";
QString Core::FolderPlugins = "xoPlugins/";
QString Core::FolderModules = "xoModules/";
QString Core::FolderCorePlugins = "xoCorePlugins/";
QString Core::FolderLaunchers = "xoLaunchers/";
QString Core::FolderScripts = "xoScripts/";

#ifdef Q_OS_WIN

const QString Core::ApplicationExtension = "exe";
const QString Core::ApplicationExtensionDot = "." + Core::ApplicationExtension;

#elif Q_OS_LINUX

const QString Core::ApplicationExtension = "";
const QString Core::ApplicationExtensionDot = "";

#elif Q_OS_MAC

const QString Core::ApplicationExtension = "app";
const QString Core::ApplicationExtensionDot = "." + Core::ApplicationExtension;

#endif

#ifdef Q_OS_WIN

const QString Core::PluginExtension = "dll";
const QString Core::PluginExtensionDot = "." + Core::PluginExtension;

#elif Q_OS_LINUX

const QString Core::luginExtension = "so";
const QString Core::luginExtensionDot = "." + Core::PluginExtension;

#elif Q_OS_MAC

const QString Core::PluginExtension = "dylib";
const QString Core::PluginExtensionDot = "." + Core::PluginExtension;

#endif

const QString Core::FileExtensionScheme = "scheme";
const QString Core::FileExtensionSchemeDot = "." + Core::FileExtensionScheme;
const QString Core::FileExtensionConfig = "config";
const QString Core::FileExtensionConfigDot = "." + Core::FileExtensionConfig;
const QString Core::FileExtensionScript = "script";
const QString Core::FileExtensionScriptDot = "." + Core::FileExtensionScript;

Core::Core(QObject *parent) : QObject(parent)
{
    m_scriptEngine = new QScriptEngine(this);
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

    if(m_loader->getApplicationStartType(moduleName) == COLD)
    {
        m_componentCountByModuleName[moduleName]++;
        m_loader->startApplication(moduleName);
    }

    return componentInfo;
}

bool Core::removeComponentFromScheme(ComponentInfo *componentInfo)
{
    if(m_loader->getApplicationStartType(componentInfo->parentModule) == COLD)
    {
        int& counter = m_componentCountByModuleName[componentInfo->parentModule];
        counter--;

        if(counter == 0) m_loader->killApplication(componentInfo->parentModule);
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

void Core::init(QString launchConfigPath)
{
    qRegisterMetaType<ONBPacket>("ONBPacket");

    QString appPath = QCoreApplication::applicationDirPath() + "/";

    FolderConfigs.prepend(appPath);
    FolderSchemes.prepend(appPath);
    FolderPlugins.prepend(appPath);
    FolderModules.prepend(appPath);
    FolderCorePlugins.prepend(appPath);
    FolderLaunchers.prepend(appPath);
    FolderScripts.prepend(appPath);

    FileUtilities::createIfNotExists(FolderConfigs);
    FileUtilities::createIfNotExists(FolderSchemes);
    FileUtilities::createIfNotExists(FolderPlugins);
    FileUtilities::createIfNotExists(FolderModules);
    FileUtilities::createIfNotExists(FolderLaunchers);
    FileUtilities::createIfNotExists(FolderScripts);

    m_server = new Server(this);
    m_server->startListening();

    m_scheme = new Scheme(this);
    connect(m_scheme, &Scheme::loaded, this, [=]() { GlobalConsole::writeLine("Scheme loaded: " + m_scheme->getLastLoadedPath()); }, Qt::QueuedConnection);

    m_hub = new Hub(this);
    m_hub->setScheme(m_scheme);

    connect(m_hub, &Hub::enableChanged, this, [=](bool enabled) { GlobalConsole::writeLine(QString("Scheme ") + (enabled ? "started" : "stopped")); }, Qt::QueuedConnection);

    m_loader = new Loader(m_server, m_hub, this);
    m_loader->load(launchConfigPath);

    auto localServer = new QLocalServer(this);
    if(!localServer->listen("xorde_local"))
    {
        QMessageBox::critical(nullptr, "Server error", "Unable to start server:" + localServer->errorString());
        localServer->close();
        return;
    }

    connect(localServer, &QLocalServer::newConnection, this, [=]()
    {
        auto socket = localServer->nextPendingConnection();
        connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);
        connect(socket, &QLocalSocket::readyRead, socket, [=](){ if(socket->readAll() == "close") QCoreApplication::quit(); });
    });
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
    return m_loader;
}

QScriptEngine *Core::getEngine()
{
    return m_scriptEngine;
}

QString Core::executeJavaScript(const QString &text)
{
    if (m_scriptEngine)
    {
        return m_scriptEngine->evaluate(text).toString();
    }
    return "?";
}

Scheme *Core::getScheme()
{
    return m_scheme;
}
