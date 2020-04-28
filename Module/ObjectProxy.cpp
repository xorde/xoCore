#include "ObjectProxy.h"
#include "ComponentProxyONB.h"

ObjectProxy::ObjectProxy(ComponentProxyONB *component, const ObjectDescription &desc) :
    mComponent(component)
{
    m_description = desc;
}

ObjectProxy::~ObjectProxy()
{
}

//void ObjectProxy::setDescription(const ObjectDescription &desc)
//{
//    if (desc.type == m_description.type && desc.size == m_description.size)
//    {
//        m_description = desc;
//    }
//    else
//    {
//        destroyBuffers();
//        m_description = desc;
//        allocateBuffers();
//    }
//}

bool ObjectProxy::isValid()
{
    return m_description.isValid();
}

QJsonObject ObjectProxy::getDescriptionJSON() const
{
    QJsonObject json;
    json["name"] = name();
    json["id"] = id;
    json["flags"] = m_description.flagString();
    //json["extFlags"] = extFlags;
    json["readType"] = typeName();
    json["writeType"] = typeName();
    json["readSize"] = static_cast<int>(m_description.size);
    json["writeSize"] = static_cast<int>(m_description.size);
    //! @todo: test this stuff:
    if (testExtFlag(MV_Min))
        json["min"] = min().toString();
    if (testExtFlag(MV_Max))
        json["max"] = max().toString();
    if (testExtFlag(MV_Def))
        json["def"] = def().toString();
    if (testExtFlag(MV_Step))
        json["step"] = step().toString();
    if (testExtFlag(MV_Mime))
        json["mimeType"] = m_mimeType;
    if (testExtFlag(MV_Hint))
        json["hint"] = m_hint;
    if (testExtFlag(MV_Unit))
        json["unit"] = m_unit;
    if (testExtFlag(MV_Options))
        json["options"] = m_options;
    return json;
}


bool ObjectProxy::link(ObjectProxy *publisher, ObjectProxy *subscriber)
{
    ObjectDescription &pubDesc = publisher->m_description;
    ObjectDescription &subDesc = subscriber->m_description;

    if ((pubDesc.size || pubDesc.type == Common) &&
        (pubDesc.type == subDesc.type)
         /*   && (pubDesc.size == subDesc.size || pubDesc.type == ObjectBase::Integer || pubDesc.type == ObjectBase::UInteger)*/ )
    {
        subscriber->linkTo(publisher);
    }

    // connect anyway!!
    subscriber->mLinkedPublisher = publisher;
    if (publisher->m_needTimestamp || !publisher->m_RMIP)
    {
        subscriber->m_needTimestamp = publisher->m_needTimestamp;
        connect(publisher, &ObjectProxy::received, subscriber, &ObjectProxy::send);
    }
    else
    {
        connect(publisher, &ObjectProxy::valueChanged, subscriber, &ObjectProxy::send);
    }
    return true;

}

bool ObjectProxy::unlink(ObjectProxy *publisher, ObjectProxy *subscriber)
{
    subscriber->unlink();
    subscriber->mLinkedPublisher = nullptr;

    disconnect(publisher, &ObjectProxy::received, subscriber, &ObjectProxy::send);
    disconnect(publisher, &ObjectProxy::valueChanged, subscriber, &ObjectProxy::send);
    return true;
}

void ObjectProxy::request() const
{
    mComponent->requestObject(m_description.id);
}

void ObjectProxy::send()
{
    if (mLinkedPublisher && (type() != mLinkedPublisher->type()))
    {
//        qDebug() << "[ObjectProxy] Performing type conversion...";
        setValue(mLinkedPublisher->value());
    }
    if (m_needTimestamp)
    {
        if (mLinkedPublisher)
            m_timestamp = mLinkedPublisher->m_timestamp;
        mComponent->sendTimedObject(id);
    }
    else
    {
        mComponent->sendObject(id);
    }
}
//---------------------------------------------------------

void ObjectProxy::subscribe(int period_ms)
{
    mComponent->subscribe(name(), period_ms);
}

void ObjectProxy::unsubscribe()
{
    mComponent->unsubscribe(name());
}
//---------------------------------------------------------
