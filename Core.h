#ifndef CORE_H
#define CORE_H

#include <QObject>
#include <QJsonObject>

#include "xoCore_global.h"

#include "Hub.h"
#include "Loader.h"
#include "Server.h"
#include "Scheme.h"
#include "ModuleList.h"
#include "xoCorePlugin.h"
#include "PluginManager.h"
#include "ConfigManager.h"
#include "Module/ComponentProxyONB.h"

class XOCORESHARED_EXPORT Core : public QObject
{
    Q_OBJECT
public:
    static Core* Instance()
    {
        static Core instance;
        return &instance;
    };

    void init();

    static QString FolderConfigs;
    static QString FolderSchemes;
    static QString FolderModules;
    static QString FolderPlugins;
    static QString FolderCorePlugins;

    static const QString FileExtensionScheme;
    static const QString FileExtensionConfig;
    static const QString FileExtensionSchemeDot;
    static const QString FileExtensionConfigDot;

    static const QString ApplicationExtension;
    static const QString ApplicationExtensionDot;

    static const QString PluginExtension;
    static const QString PluginExtensionDot;

    Server *getServer();
    Scheme *getScheme();
    Hub *getHub();
    Loader* getLoader();

    bool loadScheme(QString schemePath);
    bool deleteScheme(QString schemePath);

    ComponentInfo *createComponentInScheme(QString componentType, QString moduleName);
    bool removeComponentFromScheme(ComponentInfo* componentInfo);


private:
    explicit Core(QObject *parent = nullptr);
    virtual ~Core() override;

    Server *m_server = nullptr;
    Scheme *m_scheme = nullptr;
    Hub *m_hub = nullptr;
    Loader* loader = nullptr;

    QMap<QString, int> m_componentCountByModuleName;
};

#endif // CORE_H
