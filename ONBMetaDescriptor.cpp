#include "ONBMetaDescriptor.h"
#include "ONBSettings.h"

ONBMetaDescriptor::ONBMetaDescriptor(QObject *parent) : AbstractMetaDescriptor(parent)
{
}

void ONBMetaDescriptor::set(QObject *object)
{
    auto settings = qobject_cast<ONBSettings*>(object);

    if (settings)
    {
        for(auto& channel : settings->channels)
        {
            auto description = new ONBMetaDescription(channel);
            m_properties << description;
        }
    }
}

QJsonObject ONBMetaDescriptor::saveToJson()
{
    QVariantMap map;
    for(AbstractMetaDescription* property : m_properties)
    {
        if (property->changeable)
        map.insert(property->name, qobject_cast<ONBMetaDescription*>(property)->toJsonObject());
    }
    return QJsonObject::fromVariantMap(map);
}

bool ONBMetaDescriptor::loadFromQVariantMap(const QVariantMap &map)
{
    for(AbstractMetaDescription* property : m_properties)
    {
        if (!property->changeable)
            continue;

        if (map.contains(property->name))
        {
            property->setValue(map[property->name].toMap()["value"]);
        }
    }
    return true;
}

void ONBMetaDescriptor::setClassName(QString name)
{
    m_objectName = name;
}

QString ONBMetaDescriptor::getClassName()
{
    return m_objectName;
}

