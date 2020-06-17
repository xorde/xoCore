#include "Hub.h"

#include <QThread>
#include <QObject>
#include <QSharedPointer>
#include <QtConcurrent/QtConcurrent>
#include "ConfigManager.h"
#include "Core.h"
#include "GlobalConsole.h"


Hub::Hub(QObject *parent) : QObject(parent)
{
}

void Hub::setScheme(Scheme *scheme)
{
    m_schemeConnections.clear();

    m_scheme = scheme;

    if(m_scheme) connect(m_scheme, &Scheme::componentsUpdated, m_scheme, [=]() { checkCurrentSchemeComponents(); }, Qt::UniqueConnection);

    emit schemeChanged(scheme);
}

Scheme *Hub::getScheme()
{
    return m_scheme;
}

void Hub::addModule(ModuleProxyONB *module)
{
    static unsigned short componentID = 1;

    m_moduleConnections << connect(module, &ModuleProxyONB::newComponent, [=]()
    {
        qDebug() << "COMP NEW in module" << module->name();
        module->assignComponentID(componentID);
        componentID++;
    });

    m_moduleConnections << connect(module, &ModuleProxyONB::componentAdded, [=](unsigned short componentID)
    {
        auto component = module->component(componentID);

        m_moduleConnections << connect(component, &ComponentProxyONB::ready, [=]()
        {
            ConfigManager::writeComponentConfig(module, component);
            componentsByName[component->componentName()] = component;
            emit componentAdded(component);
            qDebug() << "COMP ADDED" << component->componentName();
            reloadComponentSettingsFromScheme(component->componentName());

            linkComponentConnections(component, m_isEnabled);
        });

        m_moduleConnections << connect(component, &ComponentProxyONB::infoChanged, [=]()
        {
            qDebug() << "COMP CHANGED" << component->componentName();

            QString name = component->componentName();
            if (componentsByName[name] != component) // component was renamed
            {
                QString oldName = componentsByName.key(component);
                componentsByName.remove(oldName);
                componentsByName[name] = component;
                emit componentRenamed(oldName, name);
            }
            else
            {
                emit componentChanged(component);
            }
        });
    });

    m_moduleConnections << connect(module, &ModuleProxyONB::ready, [=]()
    {
        emit moduleReady(module);
    });

    m_moduleConnections << connect(module, &ModuleProxyONB::componentKilled, [=](unsigned short componentID)
    {
        auto component = module->component(componentID);
        QString componentName = component->componentName();
        componentsByName.remove(componentName);

        linkComponentConnections(component, false);

        emit componentKilled(component);
    });

    m_moduleConnections << connect(module, &ModuleProxyONB::message, [=](QString text, QString component)
    {
        QString sender = module->name();
        if (!component.isEmpty())
            sender += "." + component;

        GlobalConsole::writeItem(sender, text);
    });

    module->requestIcon();
    module->enumerateClasses();
    module->enumerateComponents();

    modulesByName.insert(module->name(), module);
}

void Hub::removeModule(QString name)
{
    if(!modulesByName.contains(name)) return;

    auto module = modulesByName.take(name);
    emit componentKilled(module);
    module->disconnect();

    for(auto&& componentName : module->componentNames())
    {
        linkComponentConnections(module->component(componentName), false);
        module->deleteComponent(componentName);
        componentsByName.remove(componentName);
    }

    delete module;
}

void Hub::setIsEnabled(bool enabled)
{
    m_isEnabled = enabled;

    emit enableChanged(enabled);

    //Unlinking anyway
    for(auto& connection : m_scheme->connections) linkConnection(connection, false);

    if (m_isEnabled)
    {
        for(auto connection : m_scheme->connections)
        {
            linkConnection(connection, m_isEnabled && connection->isEnabled);
        }       
    }
}

void Hub::linkConnection(ComponentConnection *connection, bool shouldConnect)
{
    auto compOut = getComponentByName(connection->outputComponentName);
    auto compIn = getComponentByName(connection->inputComponentName);
    if (compOut && compIn)
    {
        auto objOut = compOut->object(connection->outputName);
        auto objIn = compIn->object(connection->inputName);

        if (objOut && objIn)
        {
            if(m_isEnabled && shouldConnect)
            {
                int RMIP = (connection->RMIP > 0) ? connection->RMIP : objOut->RMIP;
                ObjectProxy::link(objOut, objIn);
                compOut->subscribe(connection->outputName, RMIP);
            }
            else
            {
                ObjectProxy::unlink(objOut, objIn);
                compOut->unsubscribe(connection->outputName);
            }
        }
    }
}

bool Hub::isEnabled()
{
    return m_isEnabled;
}

QList<ModuleProxyONB *> Hub::getModules()
{
    return modulesByName.values();
}

ModuleProxyONB *Hub::getModuleByName(const QString &name)
{
    for(auto module : modulesByName.values())
    {
        if (module->name().compare(name, Qt::CaseInsensitive) == 0)
        {
            return module;
        }
    }
    return nullptr;
}

ComponentProxyONB *Hub::getComponentByName(QString name)
{
    return componentsByName.contains(name) ? componentsByName[name] : nullptr;
}

void Hub::clearConnections()
{
    m_schemeConnections.clear();
}

void Hub::checkCurrentSchemeComponents()
{
    if (!m_scheme)
    {
        for(auto module : getModules())
            for (auto compName : module->componentNames())
                module->deleteComponent(compName);
        return;
    }

    QMap<QString, int> componentCountByAppName;

    //Creating the search list
    QList<String3> schemeComponents;
    for (auto compInfo : m_scheme->components)
    {
        if (!compInfo) continue;

        schemeComponents << String3(compInfo->name, compInfo->type, compInfo->parentModule);

        componentCountByAppName[compInfo->parentModule]++;
    }

    auto startComponents = [this](QList<String3>& schemeComponents)
    {
        for(auto compHeader : schemeComponents)
        {
            auto module = getModuleByName(compHeader.moduleName);

            if (module)
                module->createComponent(compHeader.componentType, compHeader.componentName);
        }
    };

    auto loader = Core::Instance()->getLoader();
    for(auto moduleName : componentCountByAppName.keys())
    {
        if(componentCountByAppName.value(moduleName) > 0)
        {
            loader->startApplication(moduleName);
            startComponents(schemeComponents);
        }
        else
            loader->killApplication(moduleName);
    }

    for(auto module : getModules())
    {
        for (auto compName : module->componentNames())
        {
            auto compExisting = module->component(compName);
            if (compExisting)
            {
                String3 compExistingHeader(compName, compExisting->componentType(), module->name());

                bool exists = false;
                for (int i = 0; i < schemeComponents.count(); i++)
                {
                    auto compSchemeHeader = schemeComponents.at(i);
                    if (compSchemeHeader.equals(compExistingHeader))
                    {
                        schemeComponents.removeAt(i);
                        exists = true;
                        break;
                    }
                }

                if (!exists) module->deleteComponent(compName);
            }
        }
    }

    startComponents(schemeComponents);

    for(auto compInfo : m_scheme->components) reloadComponentSettingsFromScheme(compInfo);

    setIsEnabled(m_isEnabled);
}

void Hub::reloadComponentSettingsFromScheme(QString compName)
{
    if (!m_scheme) return;

    reloadComponentSettingsFromScheme(m_scheme->components[compName]);
}

void Hub::reloadComponentSettingsFromScheme(ComponentInfo *info)
{
    if (!info) return;

    auto component = getComponentByName(info->name);
    if (component)
    {
        auto settingsToLoad = info->settings;
        ONBSettings onbsettings(component);
        onbsettings.metaDescriptor()->loadFromJson(settingsToLoad);
    }
}

void Hub::linkComponentConnections(ComponentProxyONB *component, bool shouldConnect)
{
    for(auto&& output : component->getOutputs())
        for(auto connection : m_scheme->connectionsByOutput[component->componentName() + "_" + output->name()])
            linkConnection(connection, shouldConnect);

    for(auto&& input : component->getInputs())
        for(auto connection : m_scheme->connectionsByInput[component->componentName() + "_" + input->name()])
            linkConnection(connection, shouldConnect);
}
