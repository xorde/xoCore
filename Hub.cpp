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

Hub::~Hub()
{
    for(auto connection : connections) disconnect(connection);
}

void Hub::setScheme(Scheme *scheme)
{
    m_schemeConnections.clear();

    m_scheme = scheme;

    connections << connect(m_scheme, &Scheme::componentsUpdated, [=]() { checkCurrentSchemeComponents(); });

    componentConnections.clear();

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
        GlobalConsole::writeLine(QString("MODULE READY ") + module->name());

        emit moduleReady(module);
        emit componentAdded(module);
    });

    m_moduleConnections << connect(module, &ModuleProxyONB::componentKilled, [=](unsigned short componentID)
    {
        auto component = module->component(componentID);
        QString componentName = component->componentName();
        componentsByName.remove(componentName);
        emit componentKilled(component);
    });

    m_moduleConnections << connect(module, &ModuleProxyONB::message, [=](QString text, QString component)
    {
        QString sender = module->name();
        if (!component.isEmpty())
            sender += "." + component;
//        qDebug() << "MESSAGE from " << sender << ":" << text;
        GlobalConsole::writeItem(sender, text);
    });

    module->requestIcon();
    module->enumerateClasses();
    module->enumerateComponents();

    modulesById.insert(modulesById.count(), module);
}

void Hub::removeModule(int moduleId)
{
    auto module = modulesById.take(moduleId);
    emit componentKilled(module);
    disconnect(module, nullptr, nullptr, nullptr);
    delete module;
}

void Hub::setIsEnabled(bool enabled)
{
    m_isEnabled = enabled;

    emit enableChanged(enabled);

    //Unlinking anyway
    for(auto& connection : m_scheme->connections)
    {
        linkConnection(connection, false);
    }

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
                int RMIP = (connection->RMIP > 0) ? connection->RMIP : objOut->RMIP();
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
    return modulesById.values();
}

ModuleProxyONB *Hub::getModuleByName(const QString &name)
{
    for(auto module : modulesById.values())
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

ComponentBase *pluginByName(QList<ComponentBase*> plugins, QString &name)
{
    foreach(ComponentBase *comp, plugins)
    {
        if (comp->objectName().compare(name, Qt::CaseInsensitive) == 0)
        {
            return comp;
        }
    }
    return nullptr;
}

void Hub::checkCurrentSchemeComponents()
{

    if (!m_scheme)
    {
        //Removing all components
        for(auto module : getModules())
        {
            for (auto compName : module->componentNames())
            {
                module->deleteComponent(compName);
            }
        }
        return;
    }

    QMap<QString, int> componentCountByAppName;

    //Creating the search list
    QList<String4> schemeComponents;
    for (auto compInfo : m_scheme->components)
    {

        schemeComponents << String4(compInfo->name, compInfo->type, compInfo->parentModule);

        componentCountByAppName[compInfo->parentModule]++;
    }

    for(auto moduleName : componentCountByAppName.keys())
    {
        if(componentCountByAppName.value(moduleName) > 0)
            Core::Instance()->startApplication(moduleName);
        else
            Core::Instance()->killApplication(moduleName);
    }

    //Looking through existing created components
    for(auto module : getModules())
    {
        for (auto compName : module->componentNames())
        {
            //Looking for a match (name, type, module) in the scheme
            auto compExisting = module->component(compName);
            if (compExisting)
            {
                String4 compExistingHeader(compName, compExisting->componentType(), module->name());

                bool exists = false;
                for (int i = 0; i < schemeComponents.count(); i++)
                {
                    auto compSchemeHeader = schemeComponents.at(i);
                    if (compSchemeHeader.equals(compExistingHeader))
                    {
                        //Remove from search list.
                        //Found corresponding component
                        schemeComponents.removeAt(i);
                        exists = true;
                        break;
                    }
                }

                if (!exists)
                {
                    //No match found -> remove the actual component
                    module->deleteComponent(compName);
                }
            }
        }
    }

    //The leftovers in the search list should be created
    for(auto compHeader : schemeComponents)
    {
        auto module = getModuleByName(compHeader.c);
        if (module)
        {
            module->createComponent(compHeader.b, compHeader.a);
        }
    }

    //Applying settings
    for(auto compInfo : m_scheme->components)
    {
        reloadComponentSettingsFromScheme(compInfo);
    }
}

void Hub::reloadComponentSettingsFromScheme(QString compName)
{
    if (!m_scheme) return;

    auto compInfo = m_scheme->componentInfoByName(compName);

    reloadComponentSettingsFromScheme(compInfo);
}

void Hub::reloadComponentSettingsFromScheme(ComponentInfo *compInfo)
{
    if (!compInfo) return;

    auto comp = getComponentByName(compInfo->name);
    if (comp)
    {
        auto settingsToLoad = compInfo->settings;
        ONBSettings onbsettings(comp);
        onbsettings.metaDescriptor()->loadFromJson(settingsToLoad);
    }
}
