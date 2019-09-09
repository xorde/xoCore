#include "ObjectProxy.h"
#include "ComponentProxyONB.h"

ObjectProxy::ObjectProxy(ComponentProxyONB *component, const ObjectDescription &desc) :
    ObjectInfo(desc),
    mComponent(component)
{
    allocateBuffers();
}

ObjectProxy::~ObjectProxy()
{
    destroyBuffers();
}

void ObjectProxy::setDescription(const ObjectDescription &desc)
{
    if (desc.wType == mDesc.wType && desc.writeSize == mDesc.writeSize &&
        desc.rType == mDesc.rType && desc.readSize == mDesc.readSize)
    {
        mDesc = desc;
    }
    else
    {
        destroyBuffers();
        mDesc = desc;
        allocateBuffers();
    }
}

bool ObjectProxy::isValid()
{
    if (!mDesc.isValid())
        return false;
    return (mDesc.extFlags == m_extFlagsReceived);
}

QString ObjectProxy::readTypeString() const
{
    return nameOfType(static_cast<Type>(mDesc.rType));
}

QString ObjectProxy::writeTypeString() const
{
    return nameOfType(static_cast<Type>(mDesc.wType));
}

QJsonObject ObjectProxy::getDescriptionJSON() const
{
    QJsonObject json;
    json["name"] = mDesc.name;
    json["id"] = mDesc.id;
    json["flags"] = mDesc.flagString();
    //json["extFlags"] = extFlags;
    json["readType"] = readTypeString();
    json["writeType"] = writeTypeString();
    json["readSize"] = static_cast<int>(mDesc.readSize);
    json["writeSize"] = static_cast<int>(mDesc.writeSize);
    if (mMin.isValid())
        json["min"] = mMin.toString();
    if (mMax.isValid())
        json["max"] = mMax.toString();
    if (mDef.isValid())
        json["def"] = mDef.toString();
    if (mStep.isValid())
        json["step"] = mStep.toString();
    if (!m_mimeType.isEmpty())
        json["mimeType"] = m_mimeType;
    if (!m_hint.isEmpty())
        json["hint"] = m_hint;
    if (!m_unit.isEmpty())
        json["unit"] = m_unit;
    if (!m_options.isEmpty())
        json["options"] = m_options;
    return json;
}

QVariant ObjectProxy::toVariant(const void *ptr) const
{
    if (!ptr)
        return QVariant(); // "Invalid" type
    if (mDesc.wType == Common && mDesc.writeSize)
        return QByteArray(reinterpret_cast<const char*>(ptr), mDesc.writeSize);
    if (mDesc.wType == String)
        return *reinterpret_cast<const QString*>(ptr);

    if (mDesc.flags & Array)
    {
        int sz = sizeofType((Type)mDesc.wType);
        if (sz)
        {
            int N = mDesc.writeSize / sz;
            QList<QVariant> vec;
            for (int i=0; i<N; i++)
                vec << QVariant(mDesc.wType, reinterpret_cast<const char*>(ptr) + sz*i);
            return vec;
        }
        return QVariant();
    }

    QVariant result = QVariant(mDesc.wType, ptr);
    return result;
}

bool ObjectProxy::fromVariant(QVariant v, void *ptr)
{
    if (mDesc.rType != v.type())
        return false;
    if (mDesc.rType == String)
    {
        QString src = v.toString();
        QString &dst = *reinterpret_cast<QString*>(ptr);
        m_changed |= (dst != src);
        dst = src;
        return true;
    }
    else if (mDesc.rType == Common && mDesc.readSize)
    {
        QByteArray ba = v.toByteArray();
        size_t sz = ba.size();
        sz = sz > mDesc.readSize? mDesc.readSize: sz;
        m_changed |= (bool)memcmp(ptr, ba.data(), sz);
        memcpy(ptr, ba.data(), sz);
        return true;
    }
    else if (mDesc.rType == Common)
    {
        QByteArray src = v.toByteArray();
        QByteArray &dst = *reinterpret_cast<QByteArray*>(ptr);
        m_changed |= (dst != src);
        dst = src;
        return true;
    }
    if (mDesc.flags & Array)
    {
        QVariantList list = v.toList();
        int sz = sizeofType((Type)mDesc.rType);
        int N = mDesc.readSize / sz;
        for (int j=0; j < N && j < list.size(); j++)
        {
            QVariant v = list[j];
            void *dst = reinterpret_cast<unsigned char*>(ptr)+j*sz;
            m_changed |= (bool)memcmp(dst, v.data(), mDesc.readSize);
            memcpy(dst, v.data(), mDesc.readSize);
        }
    }
    else
    {
        m_changed |= (bool)memcmp(ptr, v.data(), mDesc.readSize);
        memcpy(ptr, v.data(), mDesc.readSize);
    }
    return true;
}

QVariant ObjectProxy::toVariant(const QByteArray &ba) const
{
    if (mDesc.wType == String)
        return QString::fromUtf8(ba);
    if (mDesc.wType == Common && isEnum())
        return *reinterpret_cast<const int*>(ba.constData());
    if (mDesc.wType == Common)
        return ba;
    return toVariant(ba.constData());
}

QVariant ObjectProxy::value() const
{
//    // NOTE: the value returned is the last requested value, not real time!
//    request();
    return toVariant(mWritePtr);
}

void ObjectProxy::setValue(QVariant v)
{
    if (!v.convert(mDesc.rType))
        return;
    fromVariant(v, const_cast<void *>(mReadPtr));
    if (m_changed)
        send();
}

//bool ObjectProxy::link(ObjectProxy *other)
//{
//    if (mDesc.writeSize || mDesc.wType == Common)
//        return ObjectProxy::link(this, other);
//    else if (mDesc.readSize || mDesc.rType == Common)
//        return ObjectProxy::link(other, this);
//    return false;
//}

bool ObjectProxy::link(ObjectProxy *publisher, ObjectProxy *subscriber)
{
    ObjectDescription &pubDesc = publisher->mDesc;
    ObjectDescription &subDesc = subscriber->mDesc;

    if ((pubDesc.writeSize || pubDesc.wType == Common) &&
        (pubDesc.wType == subDesc.rType) &&
        (pubDesc.writeSize == subDesc.readSize))
    {
        subscriber->mLinkedPublisher = publisher;
        subscriber->mReadPtr = publisher->mWritePtr;
        if (publisher->m_extInfo.needTimestamp)
        {
            subscriber->m_extInfo.needTimestamp = true;
            connect(publisher, &ObjectProxy::received, subscriber, &ObjectProxy::sendTimed);
        }
        else if (!publisher->m_extInfo.RMIP)
        {
            connect(publisher, &ObjectProxy::received, subscriber, &ObjectProxy::send);
        }
        else
        {
            connect(publisher, &ObjectProxy::valueChanged, subscriber, &ObjectProxy::send);
        }
        return true;
    }
    return false;
}

bool ObjectProxy::unlink(ObjectProxy *publisher, ObjectProxy *subscriber)
{
    if (subscriber->mReadPtr != publisher->mWritePtr)
        return false;
    subscriber->mReadPtr = subscriber->mDefaultReadPtr;
    disconnect(publisher, &ObjectProxy::received, subscriber, &ObjectProxy::sendTimed);
    disconnect(publisher, &ObjectProxy::valueChanged, subscriber, &ObjectProxy::send);
    return true;
}

void ObjectProxy::request() const
{
    mComponent->requestObject(mDesc.id);
}

void ObjectProxy::send() const
{
    mComponent->sendObject(mDesc.id);
}

void ObjectProxy::sendTimed()
{
    if (mLinkedPublisher)
        m_timestamp = mLinkedPublisher->m_timestamp;
    mComponent->sendTimedObject(mDesc.id);
}

bool ObjectProxy::writeMeta(const QByteArray &ba, ObjectInfo::MetaValue id)
{
    bool changed = false;
    switch (id)
    {
    case MV_Min: mMin = toVariant(ba); break;
    case MV_Max: mMax = toVariant(ba); break;
    case MV_Def: mDef = toVariant(ba); break;
    case MV_Step: mStep = toVariant(ba); break;
    case MV_Mime: m_mimeType = QString::fromUtf8(ba.constData()); break;
    case MV_Hint: m_hint = QString::fromUtf8(ba.constData()); break;
    case MV_Unit: m_unit = QString::fromUtf8(ba.constData()); break;
    case MV_Options: m_options = QString::fromUtf8(ba.constData()); break;
    case MV_ExtInfo: m_extInfo = *reinterpret_cast<const ExtendedInfo*>(ba.constData()); break;
    case MV_Enum: m_enum = QString::fromUtf8(ba.constData()).split(","); break;
    }
    unsigned short mask = (1 << (id - 1));
    if (!(m_extFlagsReceived & mask))
    {
        m_extFlagsReceived |= mask;
        changed = true;
    }
    return changed;
}

void ObjectProxy::subscribe(int period_ms)
{
    mComponent->subscribe(mDesc.name, period_ms);
}

void ObjectProxy::unsubscribe()
{
    mComponent->unsubscribe(mDesc.name);
}
//---------------------------------------------------------

void *ObjectProxy::_allocate(Type t, unsigned short sz)
{
    if (t == ObjectInfo::String)
        return new QString;
    if (t == ObjectInfo::Common && sz == 0)
        return new QByteArray;
    return new unsigned char[sz];
}

void ObjectProxy::_free(void *ptr, ObjectInfo::Type t, unsigned short sz)
{
    if (t == ObjectInfo::String)
        delete reinterpret_cast<QString*>(ptr);
    else if (t == ObjectInfo::Common && sz == 0)
        delete reinterpret_cast<QByteArray*>(ptr);
    else
        delete [] reinterpret_cast<unsigned char*>(ptr);
}

void ObjectProxy::allocateBuffers()
{
    if (!mDesc.isValid())
        return;

    if (!mWritePtr && (mDesc.writeSize || mDesc.wType == ObjectInfo::Common))
    {
        mWritePtr = _allocate(static_cast<Type>(mDesc.wType), mDesc.writeSize);
    }

    if (!mReadPtr && (mDesc.readSize || mDesc.rType == ObjectInfo::Common))
    {
        if (!mWritePtr || mDesc.flags & ObjectInfo::Dual)
            mReadPtr = _allocate(static_cast<Type>(mDesc.rType), mDesc.readSize);
        else
            mReadPtr = mWritePtr;
        mDefaultReadPtr = mReadPtr;
    }
}

void ObjectProxy::destroyBuffers()
{
    if (mWritePtr)
        _free(mWritePtr, static_cast<Type>(mDesc.wType), mDesc.writeSize);
    if (mReadPtr != mWritePtr)
        _free(const_cast<void*>(mReadPtr), static_cast<Type>(mDesc.rType), mDesc.readSize);
    mWritePtr = nullptr;
    mReadPtr = nullptr;
    mDefaultReadPtr = mReadPtr;
}
