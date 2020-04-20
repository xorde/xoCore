#include "xoCorePlugin.h"
#include <QMetaObject>
#include <QMetaProperty>

QMap<QString, QVariant> xoCorePlugin::getSettings()
{
    const QMetaObject *metaobject = this->metaObject();
    int count = metaobject->propertyCount();

    QMap<QString, QVariant> settings;

    static const QString objectNameString = "objectName";

    for (int i = 0; i < count; ++i)
    {
        QMetaProperty metaproperty = metaobject->property(i);

        if (objectNameString.compare(metaproperty.name(), Qt::CaseInsensitive) == 0 || !metaproperty.isWritable())
            continue;

        settings.insert(metaproperty.name(), metaproperty.read(this));
    }

    return settings;
}
