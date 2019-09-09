#include "Scheme.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTextStream>
#include <QDebug>
#include <QFileInfo>
#include <QTextCodec>

Scheme::Scheme()
{

}

Scheme::Scheme(const QJsonObject& in_obj)
{
    fromJson(in_obj);
}

Scheme::~Scheme()
{
    clear();
}

void Scheme::fromJson(const QJsonObject& in_obj)
{
    QJsonArray arrayComponents = in_obj.value("components").toArray();
    for(const auto value : arrayComponents)
    {
        QJsonObject object = value.toObject();
        auto compInfo = ComponentInfo::fromJson(object);
        if (compInfo != nullptr)
        {
            components << compInfo;
        }
    }

    QJsonArray arrayConnections = in_obj.value("connections").toArray();
    for(const auto value : arrayConnections)
    {
        QJsonObject object = value.toObject();
        ComponentConnection* connection = ComponentConnection::fromJson(object);
        if (connection != nullptr)
        {
            connections << connection;
            connectionsHash[connection->compoundString()] = connection;
            connectionsByOutput[connection->outputComponentName + "_" + connection->outputName].append(connection);
            connectionsByInput[connection->inputComponentName + "_" + connection->inputName].append(connection);
        }
    }
    m_description = in_obj.value("description").toString().isEmpty()? "" : in_obj.value("description").toString();

    emit componentsUpdated();
    emit connectionsUpdated();
}

QJsonObject Scheme::toJson() const
{
    QJsonObject root;
    QJsonArray arrayComponents;
    for(const auto& component : components)
    {
        arrayComponents.append(component->toJsonObject());
    }
    root.insert("components", arrayComponents);

    QJsonArray arrayConnections;
    for(const auto& connection : connections)
    {
        arrayConnections.append(connection->toJsonObject());
    }
    root.insert("connections", arrayConnections);
    QJsonValue description(m_description);
    root.insert("description",description);
    return root;
}

QString Scheme::getLastLoadedPath()
{
    return m_lastLoadedPath;
}

void Scheme::save(QString path)
{
    QJsonObject root = toJson();

    QJsonDocument document;
    document.setObject(root);

    QFile file(path);    
    file.open(QIODevice::WriteOnly);

    QTextStream stream(&file);
    stream.setCodec(QTextCodec::codecForName("UTF-8"));
    stream << document.toJson();

    m_lastLoadedPath = path;
}

bool Scheme::load(QString path)
{
    name = QFileInfo(path).baseName();

    clear(false);

    QFile file(path);
    if (!file.exists())
        return false;

    if (!file.open(QIODevice::ReadOnly))
        return false;

    auto content = file.readAll();
//    qDebug() << "Content" << content;
    QJsonDocument document = QJsonDocument::fromJson(content);
    fromJson(document.object());

    m_lastLoadedPath = path;

    //TODO: check for not working connections

    emit loaded();
    return true;
}

void Scheme::addComponent(ComponentInfo *comp)
{
    components.append(comp);
    emit componentsUpdated();    
}

void Scheme::addConnection(ComponentConnection *connection)
{
    connections.append(connection);
    connectionsHash[connection->compoundString()] = connection;
    connectionsByOutput[connection->outputComponentName + "_" + connection->outputName].append(connection);
    connectionsByInput[connection->inputComponentName + "_" + connection->inputName].append(connection);
    emit connectionsUpdated();
}

void Scheme::removeConnection(ComponentConnection *connection)
{
    connections.removeAt(connections.indexOf(connection));
    connectionsHash.remove(connection->compoundString());
    connectionsByOutput.remove(connection->outputComponentName + "_" + connection->outputName);
    connectionsByInput.remove(connection->inputComponentName + "_" + connection->inputName);
    emit connectionsUpdated();
}

ComponentInfo *Scheme::componentInfoByName(QString componentName)
{
    foreach(auto comp, components)
    {
        if (comp->name.compare(componentName, Qt::CaseInsensitive) == 0)
        {           
            return comp;            
        }
    }
    return nullptr;
}

bool Scheme::containsComponentName(QString name)
{
    return componentInfoByName(name) != nullptr;
}

bool Scheme::renameComponentByName(QString componentName,QString newName)
{
    auto comp = componentInfoByName(componentName);
    if (comp)
    {
        int index = components.indexOf(comp);
        if (index >= 0)
        {
            components.at(index)->name = newName;

            bool anyConnections = false;
            for(int i = connections.count()-1; i>=0; i--)
            {
                auto conn = connections.at(i);
                if (conn->inputComponentName.compare(componentName, Qt::CaseInsensitive) == 0)
                {
                    conn->inputComponentName = newName;
                    anyConnections = true;
                }

                if (conn->outputComponentName.compare(componentName, Qt::CaseInsensitive) == 0)
                {
                    conn->outputComponentName = newName;
                    anyConnections = true;
                }
            }

            emit componentsUpdated();
            if (anyConnections)
                emit connectionsUpdated();
            return true;
        }
    }
    return false;
}


bool Scheme::removeComponentByName(QString componentName)
{
    auto component = componentInfoByName(componentName);

    components.removeAt(components.indexOf(component));

    auto removeConnections = [=](QString componentName, QString channelName, QHash<QString, QList<ComponentConnection*>>& connections)
    {
        QString key = componentName + "_" + channelName;
        if(connections.contains(key))
            for(auto& connection : connections[key])
                removeConnection(connection);
    };

    for(auto& input : component->inputsWithType.keys())
        removeConnections(componentName, input, connectionsByInput);

    for(auto& output : component->outputsWithType.keys())
        removeConnections(componentName, output, connectionsByOutput);

    emit componentsUpdated();
    emit connectionsUpdated();

    return true;

}

ComponentInfo *Scheme::componentInfoByNameTypeModule(QString componentName, QString componentType, QString moduleName)
{
    foreach(auto comp, components)
    {
        if (comp->name.compare(componentName, Qt::CaseInsensitive) == 0 &&
            comp->type.compare(componentType, Qt::CaseInsensitive) == 0 &&
            comp->parentModule.compare(moduleName, Qt::CaseInsensitive) == 0)
        {
            return comp;
        }
    }
    return nullptr;
}

void Scheme::clear(bool sendSignals)
{
    qDeleteAll(connections);

    components.clear();
    connections.clear();
    connectionsHash.clear();
    connectionsByOutput.clear();
    connectionsByInput.clear();

    m_lastLoadedPath = "";
    m_description = "";

    if (sendSignals)
    {
        emit componentsUpdated();
        emit connectionsUpdated();
        emit cleared();
    }
}

void Scheme::reload()
{
    auto path = getLastLoadedPath();
    if (!path.isEmpty() && QFile::exists(path))
    {
        load(getLastLoadedPath());
    }
}
