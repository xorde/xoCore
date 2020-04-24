#include "PluginManager.h"

#include <QDir>
#include <QJsonDocument>
#include <QPluginLoader>

PluginManager *PluginManager::instance = nullptr;

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
    if (instance)
    {
        delete instance;
        instance = nullptr;
    }
}

QJsonObject PluginManager::parseComponents()
{
    QJsonObject json;
    for (auto module : factories)
    {
        if (!module) continue;

        QJsonObject moduleObject = module->getJsonConfig();

        for(auto componentRef : moduleObject["components"].toArray())
        {
            auto component = componentRef.toObject();

            json[component["className"].toString()] = component;
        }
    }

    return json;
}

PluginManager* PluginManager::Instance()
{
    if (!instance) instance = new PluginManager();

    return instance;
}

ModuleBaseONB* PluginManager::load(QString moduleName, QString path)
{
    QPluginLoader loader(path);
    auto plugin = loader.instance();
    if(!plugin) return nullptr;

    auto pluginModule = qobject_cast<ModuleBaseAppONB*>(plugin);
    if (pluginModule)
    {
        pluginModule->tryConnect();
        factories[moduleName] = pluginModule;
    }

    return pluginModule;
}
