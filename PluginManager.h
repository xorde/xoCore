#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include "ModuleBaseAppONB.h"
#include <QObject>
#include "xoCore_global.h"

class XOCORESHARED_EXPORT PluginManager : public QObject
{
    Q_OBJECT
public:
    explicit PluginManager(QObject *parent = nullptr);
    ~PluginManager();

    static PluginManager *Instance();
    static void removeManager();
    ModuleBaseONB *load(QString moduleName, QString path);

    QHash<QString, ModuleBaseAppONB*> factories;
    QJsonObject parseComponents();
    void createConfigList(const QJsonObject& in_obj);
private:
    static PluginManager* st_instance;    
};

#endif // PLUGINMANAGER_H
