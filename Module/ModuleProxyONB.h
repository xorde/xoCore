#ifndef MODULEPROXYONB_H
#define MODULEPROXYONB_H

#include <QObject>
#include <QTimer>
#include "ModuleConfig.h"
#include "ComponentProxyONB.h"
#include "xoCore_global.h"

class XOCORESHARED_EXPORT ModuleProxyONB : public QObject
{
    Q_OBJECT

public:
    Q_PROPERTY(QString name READ name)

private:
    QMap<unsigned short, ComponentProxyONB*> m_components;
    QMap<QString, ComponentProxyONB*> m_componentMap;
    QList<uint32_t> m_classes;
    QMap<QString, uint32_t> m_classMap;
    QHash<uint32_t, ComponentProxyONB*> m_classInfo;
    QString m_name;
    QByteArray m_iconData;

private slots:
    void receivePacket(const ONBPacket &packet);

    void sendPacket(const ONBPacket &packet);
    void parsePacket(const ONBPacket &packet);
    void sendClassInfoPacket(const ONBPacket &packet);
    void parseClassInfo(const ONBPacket &packet);

    void componentReady();
    void componentInfoChanged();

public:
    explicit ModuleProxyONB(QString module_name, QObject *parent = nullptr);

    ComponentProxyONB *component(unsigned short compID) const;
    ComponentProxyONB *component(QString name) const;

    ComponentProxyONB *classInfo(uint32_t cid) const;
    ComponentProxyONB *classInfo(QString name) const;

    const QList<uint32_t> &classes() const;
    Q_INVOKABLE QStringList classNames() const;
    Q_INVOKABLE QStringList componentNames() const;
    Q_INVOKABLE QStringList componentNames(uint32_t classID) const;
    Q_INVOKABLE QStringList componentNames(QString className) const;

    QString name() const {return m_name;}
    QList<ModuleConfig*> getModuleConfig(QString in_my_name = "", bool inInstances = false);
    QImage icon() {return QImage::fromData(m_iconData);}
    const QByteArray &iconData() const {return m_iconData;}

    void enumerateClasses();
    void enumerateComponents();
    void requestIcon();

    void receiveData(const QByteArray &data);
    void assignComponentID(unsigned short compID);

    bool createComponent(uint32_t classID, QString name = QString());
    bool deleteComponent(unsigned short compID);

    Q_INVOKABLE bool createComponent(QString className, QString componentName);
    Q_INVOKABLE bool renameComponent(QString name, QString newName);

signals:
    void newPacket(const ONBPacket &packet);
    void newDataToSend(const QByteArray &data);

    void ready();

    //! The signal is emitting when new component is found.
    //! You must call assignComponentID() with unused ComponentID as argument
    void newComponent();
    void componentAdded(unsigned short compID);
    void componentKilled(unsigned short compID);

    void message(QString text, QString component=QString());

public slots:
    bool createComponent(QString className);
    bool deleteComponent(QString name);
};

#endif // MODULEPROXYONB_H
