#include "ComponentInfo.h"


ComponentInfo::ComponentInfo(QObject *parent) : QObject(parent)
{

}

QJsonObject ComponentInfo::toJsonObject()
{
    QJsonObject componentObject;
    componentObject.insert("type",type);
    componentObject.insert("name",name);
    componentObject.insert("module",parentModule);
    componentObject.insert("enabled",enabled);
    componentObject.insert("x",visualX);
    componentObject.insert("y",visualY);
//    componentObject.insert("width",visualWidth);
//    componentObject.insert("height", visualHeight);
    componentObject.insert("settings", settings);

    componentObject.insert("inputs", jsonArrayFromStringMap(inputsWithType));
    componentObject.insert("outputs", jsonArrayFromStringMap(outputsWithType));

    return componentObject;
}

QMap<QString, QString> ComponentInfo::stringMapFromJsonArray(QJsonArray array)
{
    QMap<QString, QString> map;

    for (int i =0; i<array.count(); i++)
    {
        auto jvalue = array.at(i);
        if (jvalue.isObject())
        {
            auto jObject = jvalue.toObject();
            map.insert(jObject.value("name").toString(), jObject.value("type").toString());
        }
    }

    return map;
}

QJsonArray ComponentInfo::jsonArrayFromStringMap(QMap<QString, QString> map)
{
    QJsonArray arr;
    foreach(auto key, map.keys())
    {
        QJsonObject item;
        item.insert("name", key);
        item.insert("type", map[key]);
        arr << item;
    }
    return arr;
}

ComponentInfo *ComponentInfo::fromJson(QJsonObject &jsonObject)
{
    QString type = jsonObject.value("type").toString("");
    QString name = jsonObject.value("name").toString("");
    QString module = jsonObject.value("module").toString("");

    if (type.isEmpty() || name.isEmpty() || module.isEmpty()) return nullptr;

    ComponentInfo *componentInfo = new ComponentInfo();
    componentInfo->type = type;
    componentInfo->name = name;
    componentInfo->parentModule = module;
    componentInfo->enabled = jsonObject.value("enabled").toBool(false);

    componentInfo->visualX = jsonObject.value("x").toInt();
    componentInfo->visualY = jsonObject.value("y").toInt();
//    componentInfo->visualWidth = jsonObject.value("width").toInt();
//    componentInfo->visualHeight = jsonObject.value("height").toInt();

    componentInfo->settings = jsonObject.value("settings").toObject();

    auto inputs = jsonObject.value("inputs");
    if (inputs.isArray())
    {
        componentInfo->inputsWithType = stringMapFromJsonArray(inputs.toArray());
    }

    auto outputs = jsonObject.value("outputs");
    if (outputs.isArray())
    {
        componentInfo->outputsWithType = stringMapFromJsonArray(outputs.toArray());
    }

    return componentInfo;
}
