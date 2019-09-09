#include "PluginManager.h"

#include <QDir>
#include <QJsonDocument>
#include <QPluginLoader>

PluginManager *PluginManager::st_instance = nullptr;

PluginManager::PluginManager(QObject *parent) : QObject(parent)
{

}

PluginManager::~PluginManager()
{
    parseComponents();
    qDeleteAll(factories);
    factories.clear();
}

void PluginManager::removeManager()
{
    if (st_instance)
    {
        delete st_instance;
        st_instance = nullptr;
    }
}

QJsonObject PluginManager::parseComponents()
{
    QJsonObject json;
    for (auto module : factories)
    {
        if (module)
        {
            QJsonObject moduleObject = module->getJsonConfig();

            for(auto componentRef : moduleObject["components"].toArray())
            {
                auto component = componentRef.toObject();

                json[component["className"].toString()] = component;
            }
        }
    }
    return json;
}

PluginManager* PluginManager::Instance()
{
    if (!st_instance)
        st_instance = new PluginManager();

    return st_instance;
}

ModuleBaseONB* PluginManager::load(QString moduleName, QString path)
{
    QPluginLoader loader(path);
    QObject* plugin = loader.instance();
    if(!plugin) return nullptr;

    ModuleBaseAppONB* pluginModule = qobject_cast<ModuleBaseAppONB*>(plugin);
    if (pluginModule)
    {
        pluginModule->tryConnect();
        factories[moduleName] = pluginModule;
    }
    return pluginModule;
}
