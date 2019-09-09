#ifndef MODULELIST_H
#define MODULELIST_H

#include <QFile>
#include <QObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include "ModuleConfig.h"
#include "PluginManager.h"
#include "xoCore_global.h"

class XOCORESHARED_EXPORT ModuleList
{
public:
    QJsonObject parseComponents();
    QHash<QString, ModuleConfig*> configs;

    static ModuleList *Instance();
    static void removeList();
    void init();
    void createConfigList();
    void createConfigList(const QJsonObject& in_obj);

    void refreshConfigList();    

private:
    explicit ModuleList();
    ~ModuleList();

    static ModuleList *instance;

    QStringList getPathFiles(QString in_path);
    ModuleBaseONB* loadFactory(QString in_plugin_name);

    void parseJSON(QJsonObject root, ModuleConfig* config);
    void parse(QString moduleDirectoryPath, ModuleConfig::Type moduleType);

    void clearConfigList();    
};

#endif // MODULELIST_H
