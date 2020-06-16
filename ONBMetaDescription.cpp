#include "ONBMetaDescription.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMetaEnum>

ONBMetaDescription::ONBMetaDescription(ObjectProxy *object, QObject *parent) : AbstractMetaDescription(parent), object(object)
{
    typeName = object->typeName();
    displayName = object->name();
    name = object->name();

    hasMin = object->testExtFlag(ObjectBase::MV_Min);
    hasMax = object->testExtFlag(ObjectBase::MV_Max);
    hasDefault = object->testExtFlag(ObjectBase::MV_Def);
    hasStep = object->testExtFlag(ObjectBase::MV_Step);

    if (hasMin)
        m_min = object->min().toDouble();
    if (hasMax)
        m_max = object->max().toDouble();
    if (hasStep)
        m_step = object->step().toDouble();
    if (hasDefault)
        m_def = object->def().toString();

    suffix = object->unit();
    description = object->hint();

    auto opts = object->options();
    auto jsonOptsDoc = QJsonDocument::fromJson(opts.toUtf8());
    if (!jsonOptsDoc.isEmpty())
    {
        auto spec = jsonOptsDoc["specialType"].toString();
        if (!spec.isEmpty())
        {
            specialType = spec;
            hasSpecialType = true;
            if (spec.indexOf("pick", 0, Qt::CaseInsensitive) >= 0)
            {
                int colPos = spec.indexOf(":");
                if (colPos > 0)
                {
                    pickOption = spec.left(colPos);
                    pickExtensions = spec.mid(colPos+1);
                }
            }

            if (specialType.compare("StringList", Qt::CaseInsensitive) == 0)
            {
                auto strList = jsonOptsDoc["stringList"];
                qDebug() << strList;
                if (strList.isArray())
                {
                    auto arr =  strList.toArray();

                    foreach(auto val, arr)
                    {
                        m_enumStringList << val.toString();
                    }

                }
            }
        }
    }

    if (typeName == "int")
    {
        auto list = object->enumList();
        if (list.count() > 0 && specialType.isEmpty())
        {
            specialType = "EnumStringList";
            hasSpecialType = true;
            m_enumStringList = list;

        }
    }
    else if (typeName == "string")
    {

    }

    object->request();
    connect(object, SIGNAL(valueChanged()), this, SLOT(valueChangedSlot()));

    changeable = true;
    visibleInUI = true;
}

QMetaEnum ONBMetaDescription::enumerator()
{
    return QMetaEnum();
}

bool ONBMetaDescription::isEnumType()
{
    return false;
}

QJsonObject ONBMetaDescription::toJsonObject()
{
    QJsonObject result;
                            result["value"]       = getValue().toString();
    if(hasMin)              result["min"]         = m_min;
    if(hasMax)              result["max"]         = m_max;
    if(hasDefault)          result["default"]     = m_def;
    if(hasSpecialType)      result["specialType"] = specialType;
    if(!suffix.isEmpty())   result["suffix"]      = suffix;
    if(hasStep)             result["step"]        = m_step;
    if(!category.isEmpty()) result["category"]    = category;

    return result;
}

bool ONBMetaDescription::setValue(const QVariant &value)
{
    object->setValue(value);
    emit object->valueChanged();
    return true;
}

const QVariant ONBMetaDescription::getValue()
{
    return object->value();
}

void ONBMetaDescription::setDefault()
{
    setValue(object->def());
}

void ONBMetaDescription::valueChangedSlot()
{
    //    object->send();
    emit valueChanged(object->value());
}

bool ONBMetaDescription::setTextValue(QString value)
{
    return true;
}
