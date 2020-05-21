#ifndef COMPONENTPROXYONB_H
#define COMPONENTPROXYONB_H

#include <QObject>
#include <QVector>
#include <QImage>
#include <QDynamicPropertyChangeEvent>
#include "ObjectProxy.h"
#include "Protocol/xoTypes.h"
#include "Protocol/xoImage.h"
#include "Protocol/ONBPacket.h"
#include <QDebug>
#include "xoCore_global.h"

enum ONBChannelType
{
    Settings,
    Inputs,
    Outputs
};

class XOCORESHARED_EXPORT ComponentProxyONB : public QObject
{
Q_OBJECT
public:
    Q_PROPERTY(ulong classID READ classId)
    Q_PROPERTY(QString name READ componentName WRITE setComponentName)
    Q_PROPERTY(QString info READ componentInfo)
    Q_PROPERTY(ulong serial READ serialNumber)
    Q_PROPERTY(QString version READ versionString)
    Q_PROPERTY(QString releaseInfo READ releaseInfo)
    Q_PROPERTY(QString hardwareInfo READ hardwareInfo)
    Q_PROPERTY(int burnCount READ burnCount)
    Q_PROPERTY(uchar busType READ busType)
    Q_PROPERTY(QString type READ componentType)

    explicit ComponentProxyONB(unsigned short componentID, QObject *parent = nullptr);
    virtual ~ComponentProxyONB();

    QList<ObjectProxy*> getInputs();
    QList<ObjectProxy*> getOutputs();
    QList<ObjectProxy*> getSettings();

    QList<ObjectProxy*> getChannels(ONBChannelType type);

    QMap<QString, QString> getChannelsNameTypeMap(ONBChannelType type);

    // this is odd:
//    ObjectProxy* getInputObject(QString inputName);
//    ObjectProxy* getOutputObject(QString inputName);

    //! fill the buffer of the input with given value.
    void setInput(QString name, QVariant value);
    //! get last buffered output from component.
    Q_INVOKABLE QVariant getOutput(QString name);
    Q_INVOKABLE QVariant getSetting(QString name);

    //! send object to remote component.
    void sendObject(QString name);
    //! request object from remote component.
    Q_INVOKABLE void requestObject(QString name);

    void subscribe(QString name, int period_ms=-1); // auto-period by default
    void unsubscribe(QString name);
    void unsubscribeAll();

    unsigned short id() const { return m_id; }

    uint32_t classId() const {return m_classID;}
    QString componentName() const {return m_componentName;}
    QString componentInfo() const {return m_componentInfo;}
    uint32_t serialNumber() const { return m_serialNumber; }
    unsigned short version() const { return m_version; }
    int majorVersion() const { return m_version >> 8; }
    int minorVersion() const { return m_version & 0xFF; }
    QString versionString() const { return QString().sprintf("%d.%d", majorVersion(), minorVersion()); }
    QString releaseInfo() const { return m_releaseInfo; }
    QString hardwareInfo() const { return m_hardwareInfo; }
    int burnCount() const { return m_burnCount; }
    BusType busType() const {return static_cast<BusType>(m_busType.value());}
    QString componentType() const {return m_className;}
    const QByteArray &iconData() const {return m_iconData;}
    QImage icon() const {return QImage::fromData(m_iconData);}

    int objectCount() const {return m_objectCount;}
    const ObjectProxy *object(unsigned char oid) const {if (oid < m_objects.size()) return m_objects[oid]; return nullptr;}
    ObjectProxy *object(QString name) const {return m_objectMap.value(name, nullptr);}

    bool isReady() const {return m_ready;}
    QJsonObject getInfoJson() const;
    QJsonObject settingsJson() const;
    void setComponentName(QString name);

    void extractPrototype(ComponentProxyONB *proto) const;

    bool isFactory() const {return m_isFactory;}

signals:
    void ready();
    void infoChanged();
    void objectReceived(QString name);
    void objectChanged(QString name);

public slots:
    void requestInfo();

private:
    QVector<ObjectProxy*> m_objects;
    QMap<QString, ObjectProxy*> m_objectMap;
    QVector<QByteArray> m_objBuffers;
    QVector<ObjectBase*> m_svcObjects;

    bool m_componentInfoValid;
    bool m_objectsInfoValid;
    bool m_ready;
    bool m_isFactory = true;

    //! unique component id (aka address)
    unsigned short m_id;

    //! meta-info about the component
    xoUInt32 m_classID;
    xoString m_componentName; //!< core-side name of the component instance
    xoString m_componentInfo;
    xoUInt32 m_serialNumber;
    xoUInt16 m_version;
    xoString m_releaseInfo;
    xoString m_hardwareInfo;
    xoUInt32 m_burnCount;
    xoUInt8 m_objectCount;
    xoUInt8 m_busType;
    xoString m_className;
    xoByteArray m_iconData;

    //! create bindings for service objects
    unsigned char bindSvcObject(ObjectBase &obj);

    //! this is ObjectProxy factory - important thing
    ObjectProxy *createObject(const ObjectDescription &desc);
    //! create ObjectInfo with bindings to local buffer
    void prepareObject(const ObjectDescription &desc);
    //! check if ObjectInfo description read completed (and emit the signal)
    void checkForReady();

    void parseServiceMessage(unsigned char oid, const QByteArray &data);
    void parseMessage(unsigned char oid, const QByteArray &data);
    void sendServiceMessage(unsigned char oid, const QByteArray &data = QByteArray());
    void sendServiceMessage(unsigned char oid, unsigned char data);
    void sendMessage(unsigned char oid, const QByteArray &data = QByteArray());

protected:
    virtual bool event(QEvent *event);

    friend class ObjectProxy;
    void requestObject(unsigned char oid);
    void sendObject(unsigned char oid);
    void sendTimedObject(unsigned char oid);

signals: // for internal use
    void newData(const ONBPacket &packet);

private slots:
    friend class ModuleProxyONB;
    void receiveData(const ONBPacket &packet);
};

#endif // COMPONENTPROXYONB_H
