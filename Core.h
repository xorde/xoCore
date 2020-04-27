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
    explicit Core(QObject *parent = nullptr);
    virtual ~Core() override;

    static Core* Instance();

    static QString FolderConfigs;
    static QString FolderSchemes;
    static QString FolderModules;
    static QString FolderPlugins;
    static QString FolderCorePlugins;

    static QString FileExtensionScheme;
    static QString FileExtensionConfig;
    static QString FileExtensionSchemeDot;
    static QString FileExtensionConfigDot;

    Server *getServer();
    Scheme *getScheme();
    Hub *getHub();
    Loader* getLoader();

    bool loadScheme(QString schemePath);
    bool deleteScheme(QString schemePath);

    void loadCorePlugins();
    QMap<QString, xoCorePlugin*> getCorePlugins();

    ComponentInfo *createComponentInScheme(QString componentType, QString moduleName);
    bool removeComponentFromScheme(ComponentInfo* componentInfo);


private:
    QMap<QString, xoCorePlugin*> m_corePlugins;

    Server *m_server = nullptr;
    Scheme *m_scheme = nullptr;
    Hub *m_hub = nullptr;
    Loader* loader = nullptr;

    QMap<QString, int> m_componentCountByModuleName;

    static Core* _instance;
};

#endif // CORE_H
