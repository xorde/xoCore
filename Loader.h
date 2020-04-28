#ifndef LOADER_H
#define LOADER_H

#include <QSet>
#include <QMap>
#include <QObject>
#include <QProcess>

#include "Hub.h"
#include "Server.h"
#include "ModuleStartType.h"
#include "Module/ModuleProxyONB.h"

class XOCORESHARED_EXPORT Loader : public QObject
{
    Q_OBJECT
public:
    explicit Loader(Server* server, Hub* hub, QObject *parent = nullptr);
    ~Loader();

    bool getApplicationStartType(QString applicationName);
    void startApplication(QString applicationName);
    void killApplication(QString applicationName);
    bool applicationIsRunning(QString applicationName);
    void updateModuleStartType(QString moduleName, ModuleStartType type);
    void refreshConfigsForModule(QString moduleName);
    QString getModulePath(QString moduleName, ModuleConfig::Type type);
    void parseApplicationsStartOptions();

    QList<QString> pluginList;
    QList<QString> applicationList;

signals:
    void configWritten(ModuleProxyONB* module);

private:
    Server* server = nullptr;
    Hub* hub = nullptr;

    QSet<QString> appNames;
    QSet<QString> names;
    QMap<QString, QProcess*> processesByAppName;
    QMap<QString, ModuleProxyONB*> moduleByName;
    QMap<QString, ModuleStartType> startTypeByAppName;
    QMap<QString, QMetaObject::Connection> moduleConnectsModuleName;

};

#endif // LOADER_H
