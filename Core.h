#ifndef CORE_H
#define CORE_H

#include <QObject>
#include <QJsonObject>
#include <QProcess>
#include "ConfigManager.h"
#include "Server.h"
#include "Scheme.h"
#include "Module/ModuleProxyONB.h"
#include "Module/ComponentProxyONB.h"
#include "Hub.h"
#include "PluginManager.h"
#include "ModuleList.h"
#include "ModuleStartType.h"
#include "xoCore_global.h"
#include "xoCorePlugin.h"

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


    void startApplication(QString applicationName);
    void killApplication(QString applicationName);
    bool applicationIsRunning(QString applicationName);
    bool getApplicationStartType(QString applicationName);    
    void updateModuleStartType(QString moduleName, ModuleStartType type);
    void refreshConfigsForModule(QString moduleName);

    bool loadScheme(QString schemePath);
    bool deleteScheme(QString schemePath);

    void loadCorePlugins();
    QMap<QString, xoCorePlugin*> getCorePlugins();


    ComponentInfo *createComponentInScheme(QString componentType, QString moduleName);
    bool removeComponentFromScheme(ComponentInfo* componentInfo);

signals:
    void configWritten(ModuleProxyONB* module);

private:
    QMap<QString, xoCorePlugin*> m_corePlugins;

    Server *m_server = nullptr;
    Scheme *m_scheme = nullptr;
    Hub *m_hub = nullptr;

    QMap<QString, QProcess*> m_processesByAppName;
    QSet<QString> m_appNames;

    QSet<QString> m_names;

    QMap<QString, ModuleStartType> m_startTypeByAppName;

    void parseApplicationsStartOptions();
    QString getApplicationPath(QString applicationName);

    static Core* _instance;

    QMap<QString, QMetaObject::Connection> m_moduleConnectsModuleName;

    QMap<QString, int> m_componentCountByModuleName;
    QMap<QString, ModuleProxyONB*> m_moduleByName;
};

#endif // CORE_H
