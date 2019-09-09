#ifndef SCHEME_H
#define SCHEME_H

#include <QList>
#include "Data/ComponentInfo.h"
#include "Data/ComponentConnection.h"
#include "xoCore_global.h"

class XOCORESHARED_EXPORT Scheme : public QObject
{
    Q_OBJECT
public:
    Scheme();    
    Scheme(const QJsonObject& in_obj);

    ~Scheme();

    QString name;

    void save(QString path);
    bool load(QString path);

    void fromJson(const QJsonObject& in_obj);
    void setDescription(QString desc) {m_description = desc;}
    QString getDescription() {return m_description;}

    void addComponent(ComponentInfo *comp);
    void addConnection(ComponentConnection *conn);
    void removeConnection(ComponentConnection *connection);

    ComponentInfo *componentInfoByName(QString componentName);
    bool containsComponentName(QString name);
    bool removeComponentByName(QString componentName);
    bool renameComponentByName(QString componentName, QString newName);

    ComponentInfo *componentInfoByNameTypeModule(QString componentName, QString componentType, QString moduleName);

    void clear(bool sendSignals = true);
    void reload();

    QList<ComponentInfo*> components;
    QList<ComponentConnection*> connections;

    QHash<QString, QList<ComponentConnection*>> connectionsByOutput;
    QHash<QString, QList<ComponentConnection*>> connectionsByInput;
    QHash<QString, ComponentConnection*> connectionsHash;

    QJsonObject toJson() const;

    QString getLastLoadedPath();

protected:
    QString m_lastLoadedPath = "";
    QString m_description = "";

signals:
    void connectionsUpdated();
    void componentsUpdated();
    void cleared();
    void loaded();

};

#endif // SCHEME_H
