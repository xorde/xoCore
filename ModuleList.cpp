#include <QDir>
#include "ModuleList.h"
#include "Core.h"

ModuleList *ModuleList::instance = nullptr;

ModuleList* ModuleList::Instance()
{
    if (!instance)
        instance = new ModuleList();

    return instance;
}

void ModuleList::removeList()
{
    if (instance)
    {
        delete instance;
        instance = nullptr;
    }
}

ModuleList::~ModuleList()
{
    PluginManager::removeManager();
    clearConfigList();
}

void ModuleList::init()
{
    QStringList moduleDirectoryPaths = getPathFiles(Core::FolderModules);
    for(const auto& moduleDirectoryPath : moduleDirectoryPaths)
        parse(moduleDirectoryPath, ModuleConfig::Type::MODULE);

    QStringList pluginsDirectoryPaths = getPathFiles(Core::FolderPlugins);
    for(const auto& pluginDirectoryPath : pluginsDirectoryPaths)
        parse(pluginDirectoryPath, ModuleConfig::Type::PLUGIN);
}

QStringList ModuleList::getPathFiles(QString in_path)
{
    QDir modulesDirectory(in_path);
    return modulesDirectory.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
}

ModuleList::ModuleList()
{    
    refreshConfigList();
}

void ModuleList::parseJSON(QJsonObject root, ModuleConfig* config)
{
    QString name;
    QString type;
    QJsonArray inputs = root.value("inputs").toArray();
    for(const auto input : inputs)
    {
        QJsonObject object = input.toObject();
        name = object.value("name").toString();
        type = QVariant::typeToName(object.value("type").toString().toInt());
        config->inputs[name] = new IOChannel(name, type);
    }

    QJsonArray outputs = root.value("outputs").toArray();
    for(const auto output : outputs)
    {
        QJsonObject object = output.toObject();
        name = object.value("name").toString();
        type = QVariant::typeToName(object.value("type").toString().toInt());
        config->outputs[name] = new IOChannel(name, type);
    }
};

void ModuleList::createConfigList()
{    
    clearConfigList();
    refreshConfigList();
}

void ModuleList::createConfigList(const QJsonObject& in_obj)
{
    clearConfigList();
    QStringList keys = in_obj.keys();
    for (QString key : keys)
    {
        QJsonObject obj = in_obj[key].toObject();
        configs[key] = new ModuleConfig(obj);
    }
}

QJsonObject ModuleList::parseComponents()
{
    if (configs.isEmpty())
        refreshConfigList();

    QJsonObject obj;
    QStringList keys = configs.keys();
    for (QString name : keys)
    {
        ModuleConfig* pModuleConfig = configs[name];
        if (pModuleConfig)
            obj[name] = pModuleConfig->getJson();
    }

    return obj;
}

ModuleBaseONB* ModuleList::loadFactory(QString in_plugin_name)
{
#ifdef Q_OS_LINUX

    #ifdef QT_DEBUG
        QString file_path =  Core::Instance()->FolderPlugins + "/" + in_plugin_name + "/" + in_plugin_name + "d.so";
    #else
        QString file_path =  Core::Instance()->FolderPlugins + "/" + in_plugin_name + "/" + in_plugin_name + ".so";
    #endif

#else

    #ifdef QT_DEBUG
        QString file_path =  Core::Instance()->FolderPlugins + "/" + in_plugin_name + "/" + in_plugin_name + "d.dll";
    #else
        QString file_path =  Core::Instance()->FolderPlugins + "/" + in_plugin_name + "/" + in_plugin_name + ".dll";
    #endif

#endif
    return PluginManager::Instance()->load(in_plugin_name, file_path);
}

void ModuleList::refreshConfigList()
{    
    Core* pCore = Core::Instance();
    if (!pCore)
        return;

    auto hub = Core::Instance()->getHub();
    if (hub)
    {
        for(auto& module : hub->getModules())
        {
            QString moduleName = module->name();
            for (QString componentName : module->classNames())
            {
                QList<ModuleConfig*> list = module->getModuleConfig(moduleName);
                for (auto pConfig : list)
                    configs[pConfig->name] = pConfig;
            }
        }
    }
}

void ModuleList::parse(QString moduleDirectoryPath, ModuleConfig::Type moduleType)
{
    QString moduleName = QDir(moduleDirectoryPath).dirName();
    switch(moduleType)
    {
        case ModuleConfig::Type::MODULE:
        {

            QString moduleConfigPath = Core::FolderConfigs + moduleName + "/";
            for(auto entry : QDir(moduleConfigPath).entryList(QDir::Files))
            {
                QFile file(moduleConfigPath + entry);
                if(!file.open(QIODevice::ReadOnly)) continue;

                auto config = new ModuleConfig();
                configs[QFileInfo(entry).baseName()] = config;

                QJsonDocument document = QJsonDocument::fromJson(file.readAll());
                parseJSON(document.object(), config);
            }
            break;
        }

        case ModuleConfig::Type::PLUGIN:
        {
            ModuleBaseONB* pFactory = loadFactory(moduleName);
            if (!pFactory)
                break;

            for(auto& json : pFactory->getClassesInfo())
            {
                auto config = new ModuleConfig();
                config->name = json.value("className").toString();
                config->type = moduleType;
                config->parent = moduleName;
                parseJSON(json, config);
                configs[config->name] = config;
            }
            break;
        }
    }
};

void ModuleList::clearConfigList()
{
    qDeleteAll(configs);
    configs.clear();
}
