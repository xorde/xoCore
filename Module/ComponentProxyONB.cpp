#include "ComponentProxyONB.h"

#include <QJsonArray>

ComponentProxyONB::ComponentProxyONB(unsigned short componentID, QObject *parent) : QObject(parent),
    m_componentInfoValid(false),
    m_objectsInfoValid(false),
    m_ready(false),
    m_id(componentID),
    m_classID(0),
    m_serialNumber(0),
    m_version(0),
    m_burnCount(0),
    m_objectCount(0),
    m_busType(0)
{
    // order of service objects is predefined by ONB (to be compatible with devices)
    bindSvcObject(m_classID);
    bindSvcObject(m_componentName);
    bindSvcObject(m_componentInfo);
    bindSvcObject(m_serialNumber);
    bindSvcObject(m_version);
    bindSvcObject(m_releaseInfo);
    bindSvcObject(m_hardwareInfo);
    bindSvcObject(m_burnCount);
    bindSvcObject(m_objectCount);
    bindSvcObject(m_busType);
    bindSvcObject(m_className);
    bindSvcObject(m_iconData);
}

ComponentProxyONB::~ComponentProxyONB()
{
    qDeleteAll(m_objects);
    qDeleteAll(m_svcObjects);
}

QJsonObject ComponentProxyONB::getInfoJson() const
{
    QJsonObject obj;
    obj["classID"] = QString::number(m_classID);
    obj["type"] = m_className.value();
    obj["name"] = m_componentName.value();
    obj["info"] = m_componentInfo.value();
    obj["serial"] = QString::number(m_serialNumber);
    obj["version"] = QString::number(m_version);
    obj["releaseinfo"] = m_releaseInfo.value();
    obj["hardware"] = m_hardwareInfo.value();

    QJsonArray inputs, outputs;
    for (const ObjectProxy* pInfo : m_objects)
    {
        if (!pInfo)
            continue;
        switch(pInfo->flags())
        {
        case ObjectBase::Input:
            inputs.push_back(pInfo->createJsonInfo());
            break;
        case ObjectBase::Output:
            outputs.push_back(pInfo->createJsonInfo());
            break;
        default:; // eliminate warning
        }
    }

    obj["inputs"] = inputs;
    obj["outputs"] = outputs;

    return obj;
}

QJsonObject ComponentProxyONB::settingsJson() const
{
    QJsonObject obj;
    QJsonArray arr;
    for (const ObjectBase* pInfo : m_svcObjects)
        arr.push_back(pInfo->createJsonInfo());
    return obj;
}

bool ComponentProxyONB::event(QEvent *event)
{
    if (event->type() == QEvent::DynamicPropertyChange && isReady())
    {
        QDynamicPropertyChangeEvent *e = dynamic_cast<QDynamicPropertyChangeEvent*>(event);
        QString name = QString::fromUtf8(e->propertyName());
        qDebug() << "[ComponentProxyONB] property changed from QObject" << name;
        ObjectProxy *obj = m_objectMap.value(name, nullptr);
        if (obj)
            obj->setValue(property(e->propertyName()));
        return true;
    }
    return QObject::event(event);
}

QList<ObjectProxy *> ComponentProxyONB::getInputs()
{
    return getChannels(Inputs);
}

QList<ObjectProxy *> ComponentProxyONB::getOutputs()
{
    return getChannels(Outputs);
}

QList<ObjectProxy *> ComponentProxyONB::getSettings()
{
   return getChannels(Settings);
}

QList<ObjectProxy *> ComponentProxyONB::getChannels(ONBChannelType type)
{
    QList<ObjectProxy*> result;
    for (auto info : m_objects)
    {
        if (!info)
            continue;
        switch(type)
        {
        case Inputs:
            if (info->isReadable() && info->isVolatile())
                result << info;
            break;

        case Outputs:
            if (info->isWritable() && info->isVolatile())
                result << info;
            break;

        case Settings:
            if (info->isReadable() && info->isWritable() && !info->isVolatile() && !info->isFunction())
                result << info;
            break;
        }
    }
    return result;
}

QMap<QString, QString> ComponentProxyONB::getChannelsNameTypeMap(ONBChannelType type)
{
    QMap<QString, QString> map;
    auto channels = getChannels(type);

    foreach(auto channel, channels)
    {
        map.insert(channel->name(), channel->typeName());
    }

    return map;
}

void ComponentProxyONB::setInput(QString name, QVariant value)
{
    if (!m_objectMap.contains(name))
        return;
    ObjectProxy *obj = m_objectMap[name];
    obj->setValue(value);
}

QVariant ComponentProxyONB::getOutput(QString name)
{
    if (!m_objectMap.contains(name))
        return QVariant();
    ObjectProxy *obj = m_objectMap[name];
    QVariant v = obj->value();
    // TODO: implement internal property setting without event firing!!
    setProperty(name.toUtf8(), v);
    return v;
}

QVariant ComponentProxyONB::getSetting(QString name)
{
    if (!m_objectMap.contains(name))
        return QVariant();
    ObjectProxy *obj = m_objectMap[name];
    QVariant v = obj->value();
    // TODO: implement internal property setting without event firing!!
    setProperty(name.toUtf8(), v);
    return v;
}

void ComponentProxyONB::sendObject(unsigned char oid)
{
    if (oid < m_objects.size() && m_objects[oid])
        sendMessage(oid, m_objects[oid]->read());
}

void ComponentProxyONB::sendTimedObject(unsigned char oid)
{
    if (oid < m_objects.size())
    {
        ObjectProxy *obj = m_objects[oid];
        if (!obj)
            return;
        QByteArray ba;
        ba.append(reinterpret_cast<const char*>(&oid), sizeof(unsigned char));
        ba.append('\0'); // reserved byte
        ba.append(reinterpret_cast<const char*>(&obj->m_timestamp), sizeof(uint32_t));
        obj->read(ba);
        sendServiceMessage(svcTimedObject, ba);
    }
}

void ComponentProxyONB::sendObject(QString name)
{
    if (!m_objectMap.contains(name))
        return;

    ObjectProxy *obj = m_objectMap[name];
    //    if (value.isValid())
    //        obj->setValue(value);

    sendMessage(obj->id, obj->read());
}

void ComponentProxyONB::requestObject(unsigned char oid)
{
    if (oid < m_objects.size())
        sendMessage(oid);
}

void ComponentProxyONB::requestObject(QString name)
{
    if (!m_objectMap.contains(name))
        return;
    const ObjectBase *obj = m_objectMap[name];
    sendMessage(obj->id);
}

void ComponentProxyONB::subscribe(QString name, int period_ms)
{
    if (!m_objectMap.contains(name))
        return;
    ObjectProxy *obj = m_objectMap[name];
    if (obj->isVolatile() && obj->isWritable())
    {
        if (m_busType == BusSwonb || m_busType == BusRadio)
        {
            obj->m_autoPeriodMs = period_ms;
            obj->mAutoRequestTimer = new QTimer(obj);
            connect(obj->mAutoRequestTimer, &QTimer::timeout, obj, &ObjectProxy::request);
            obj->mAutoRequestTimer->start(obj->m_autoPeriodMs);
        }
        else
        {
            QByteArray ba;
            ba.append(reinterpret_cast<const char*>(&period_ms), sizeof(int));
            ba.append(obj->id);
            if (obj->m_needTimestamp)
                sendServiceMessage(svcTimedRequest, ba);
            else
                sendServiceMessage(svcSubscribe, ba);
        }
    }
}

void ComponentProxyONB::unsubscribe(QString name)
{
    if (!m_objectMap.contains(name))
        return;
    ObjectProxy *obj = m_objectMap[name];

    if (m_busType == BusSwonb || m_busType == BusRadio)
    {
        if (obj->mAutoRequestTimer)
        {
            obj->mAutoRequestTimer->stop();
            obj->mAutoRequestTimer->disconnect();
            obj->mAutoRequestTimer->deleteLater();
            obj->mAutoRequestTimer = nullptr;
        }
    }
    else
    {
        QByteArray ba;
        ba.append(obj->id);
        sendServiceMessage(svcUnsubscribe, ba);
    }
}

void ComponentProxyONB::unsubscribeAll()
{
    sendServiceMessage(svcUnsubscribe);
}

void ComponentProxyONB::setComponentName(QString name)
{
    //sendServiceMessage(svcName, name.toUtf8());
    m_componentName = name;
    emit infoChanged();
}

void ComponentProxyONB::extractPrototype(ComponentProxyONB *proto) const
{
    //    proto->m_componentName = "";
    proto->m_classID = m_classID.value();
    proto->m_className = QString(m_className).isEmpty() ? m_componentName.value() : m_className.value(); // !!! class name is extracted from component name !!!
    proto->m_componentInfo = m_componentInfo.value();
    proto->m_serialNumber = 0;
    proto->m_version = m_version.value();
    proto->m_releaseInfo = m_releaseInfo.value();
    proto->m_hardwareInfo = m_hardwareInfo.value();
    proto->m_burnCount = m_burnCount.value();
    proto->m_objectCount = m_objectCount.value();
    proto->m_busType = m_busType.value();
    proto->m_iconData = m_iconData.value();

    for (ObjectProxy *p: m_objects)
    {
        if (!p)
            continue;
        ObjectProxy *obj = proto->createObject(p->description());//new ObjectProxy(proto, p->description());
        proto->m_objects << obj;
        proto->m_objectMap[p->name()] = obj;
    }

    proto->m_componentInfoValid = m_componentInfoValid;
    proto->m_objectsInfoValid = m_objectsInfoValid;
    proto->m_ready = m_ready;
}
//---------------------------------------------------------

void ComponentProxyONB::requestInfo()
{
    sendServiceMessage(svcClass);
    sendServiceMessage(svcName);
    sendServiceMessage(svcRequestAllInfo);
}
//---------------------------------------------------------

void ComponentProxyONB::receiveData(const ONBPacket &packet)
{
    unsigned short compID = packet.header().componentID;
    if (compID && compID != m_id)
        return; // discard fail packet

    unsigned char oid = packet.header().objectID;
    if (packet.header().svc)
    {
        if (packet.header().local)
            parseServiceMessage(oid, packet.data());
    }
    else
        parseMessage(oid, packet.data());
}

void ComponentProxyONB::parseServiceMessage(unsigned char oid, const QByteArray &data)
{
    if (oid < m_svcObjects.size())
    {
        m_svcObjects[oid]->write(data);
    }

    if (oid == svcObjectCount)
    {
        int oldCount = m_objects.size();
        if (oldCount > m_objectCount)
        {
            for (ObjectProxy *o: m_objects)
            {
                if (o)
                    o->deleteLater();
            }
            oldCount = 0;
        }
        m_objectsInfoValid = false;
        m_objects.resize(m_objectCount);
        m_objBuffers.resize(m_objectCount);
        for (int _oid=oldCount; _oid<m_objectCount; _oid++)
        {
            m_objects[_oid] = nullptr;
            sendServiceMessage(svcObjectInfo, _oid);
        }

        //sendServiceMessage(svcRequestObjInfo);
    }
    else if (oid == svcObjectInfo)
    {
        prepareObject(data);
        unsigned char _oid = data[0];
        QByteArray ba;
        ba.append(_oid);
//        sendServiceMessage(svcObjectExtInfo, ba);
        unsigned short ef = m_objects[_oid]->description().extFlags;
        for (int i=0; i<16; i++)
        {
            if (ef & (1<<i))
                sendServiceMessage(svcObjectMinimum + i, ba);
        }
    }
    else if (oid >= svcObjectMinimum && oid < svcTimedObject)
    {
        unsigned char _oid = data[0];
        if (_oid < m_objectCount && m_objects[_oid])
        {
            ObjectProxy *obj = m_objects[_oid];
            unsigned char metavalue = oid - svcObjectInfo;
            bool changed = obj->writeMeta(data.mid(1), static_cast<ObjectBase::MetaValue>(metavalue));
            if (changed)
                checkForReady();
        }
    }
    else if (oid == svcTimedObject)
    {
        unsigned char _oid = data[0];
        uint32_t timestamp = *reinterpret_cast<const uint32_t*>(data.data() + 2);
        if (_oid < m_objects.size() && m_objects[_oid])
        {
            ObjectProxy *obj = m_objects[_oid];
            obj->m_timestamp = timestamp;
            // TODO: get rid of copying
            parseMessage(_oid, data.mid(6));
        }
    }
    else if (oid == svcFail)
    {
        if (data.size())
        {
            unsigned char failedOid = static_cast<unsigned char>(data[0]);
            switch (failedOid)
            {
            case svcRequestAllInfo:
                for (ObjectBase *obj: m_svcObjects)
                    sendServiceMessage(obj->description().id);
                break;

            case svcRequestObjInfo:
                for (unsigned char idx=0; idx<m_objectCount; idx++)
                    sendServiceMessage(svcObjectInfo, idx);
                break;

            case svcAutoRequest:
                //! @TODO: change firmware to return details
                qDebug() << "[ComponentProxyONB] subscribe failed";
                break;

            case svcTimedRequest:
                //! @TODO: create timer and set flag needTimestamp
                qDebug() << "[ComponentProxyONB] subscribe with timestamp failed";
                break;

            default:;
            }

            //! @TODO: if service object doesn't exist, just check it
            //            if (failedOid < svcObjectInfo)
            //                receiveServiceObject(failedOid, ByteArray());
        }
    }
}

void ComponentProxyONB::parseMessage(unsigned char oid, const QByteArray &data)
{
    if (oid < m_objects.size())
    {
        ObjectProxy *obj = m_objects[oid];
        if (!obj)
            return;

        obj->write(data);

        emit objectReceived(obj->name());

        if (obj->m_changed)
        {
            obj->m_changed = false;
            //            qDebug() << "object changed" << obj->name() << " to " << obj->value();
            emit objectChanged(obj->name());
        }
    }
}

void ComponentProxyONB::sendServiceMessage(unsigned char oid, const QByteArray &data)
{
    ONBHeader hdr;
    hdr.objectID = oid;
    hdr.componentID = m_id;
    hdr.svc = 1;
    hdr.local = 1;
    emit newData(ONBPacket(hdr, data));
}

void ComponentProxyONB::sendServiceMessage(unsigned char oid, unsigned char data)
{
    QByteArray ba;
    ba.append(data);
    sendServiceMessage(oid, ba);
}

void ComponentProxyONB::sendMessage(unsigned char oid, const QByteArray &data)
{
    ONBHeader hdr;
    hdr.objectID = oid;
    hdr.componentID = m_id;
    hdr.local = 1;
    emit newData(ONBPacket(hdr, data));
}

//---------------------------------------------------------

unsigned char ComponentProxyONB::bindSvcObject(ObjectBase &obj)
{
    obj.m_description.flags |= ObjectBase::ReadWrite;
    m_svcObjects << &obj;
    obj.setId(m_svcObjects.size() - 1);
    return obj.description().id;
}
//---------------------------------------------------------

ObjectProxy *ComponentProxyONB::createObject(const ObjectDescription &desc)
{
    switch (static_cast<ObjectBase::Type>(desc.type))
    {
        case ObjectBase::Bool: return new ObjectProxyImpl<bool>(this, desc);
        case ObjectBase::Int: return new ObjectProxyImpl<int>(this, desc);
        case ObjectBase::UInt: return new ObjectProxyImpl<unsigned int>(this, desc);
        case ObjectBase::LongLong: return new ObjectProxyImpl<int64_t>(this, desc);
        case ObjectBase::ULongLong: return new ObjectProxyImpl<uint64_t>(this, desc);
        case ObjectBase::Double: return new ObjectProxyImpl<double>(this, desc);
        case ObjectBase::Long: return new ObjectProxyImpl<int32_t>(this, desc);
        case ObjectBase::Short: return new ObjectProxyImpl<int16_t>(this, desc);
        case ObjectBase::Char: return new ObjectProxyImpl<char>(this, desc);
        case ObjectBase::ULong: return new ObjectProxyImpl<uint32_t>(this, desc);
        case ObjectBase::UShort: return new ObjectProxyImpl<uint16_t>(this, desc);
        case ObjectBase::UChar: return new ObjectProxyImpl<uint8_t>(this, desc);
        case ObjectBase::Float: return new ObjectProxyImpl<float>(this, desc);
        case ObjectBase::SChar: return new ObjectProxyImpl<int8_t>(this, desc);

        case ObjectBase::String: return new ObjectProxyImpl<QString>(this, desc);
        case ObjectBase::Common: return new ObjectProxyImpl<QByteArray>(this, desc);
        case ObjectBase::Variant: return new ObjectProxyImpl<QVariant>(this, desc);

        default: return nullptr;
    }
}

void ComponentProxyONB::prepareObject(const ObjectDescription &desc)
{
    unsigned char oid = desc.id;

    if (oid >= m_objectCount)
    {
        qDebug() << "[ComponentProxyONB] prepareObject() fail: objectID out of bounds";
        return;
    }

    ObjectProxy *obj = m_objects.value(oid, nullptr);
    if (obj)
    {
        m_objectMap.remove(obj->name());
//        obj->setDescription(desc);
        delete obj; //! @warning this is OPASNO!! or not, need to test
    }
//    else
//    {
        obj = createObject(desc); //new ObjectProxy(this, desc);
//    }

    if (!obj)
        qDebug("PIZDEC koro4e blya: no object created for given type");

    m_objects[oid] = obj;
    m_objectMap[desc.name] = obj;

    // TODO: new thing, need testing!!!
    if (!property(obj->name().toUtf8()).isValid())
        setProperty(obj->name().toUtf8(), QVariant(static_cast<QVariant::Type>(obj->type())));

    // check if all object info is received
    checkForReady();
}

void ComponentProxyONB::checkForReady()
{
    bool flag = true;
    for (auto& objectInfo : m_objects)
    {
        if (!objectInfo || !objectInfo->isValid())
        {
            flag = false;
            break;
        }
    }

    m_objectsInfoValid = flag;

    if (m_objectsInfoValid)
    {
        if (!m_ready)
        {
            m_ready = true;
            emit ready();
        }
        else
        {
            qDebug() << "[ComponentProxyONB] info changed";
            emit infoChanged();
        }
    }
}

