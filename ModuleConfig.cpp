#include "ModuleConfig.h"


ModuleConfig::ModuleConfig()
{

}

ModuleConfig::ModuleConfig(const QJsonObject& in_obj)
{
    parseJson(in_obj);
}

ModuleConfig::~ModuleConfig()
{
    qDeleteAll(inputs);
    qDeleteAll(outputs);
    inputs.clear();
    outputs.clear();
}

void ModuleConfig::parseJson(QJsonObject in_obj)
{
    name = in_obj["name"].toString();
    parent = in_obj["parent"].toString();

    QJsonArray j_inputs = in_obj["inputs"].toArray();
    for (QJsonValue val : j_inputs)
    {
        QJsonObject obj = val.toObject();
        QString name = obj["name"].toString();
        QString type = obj["type"].toString();
        inputs[name] = new IOChannel(name, type);
    }
    QJsonArray j_outputs = in_obj["outputs"].toArray();
    for (QJsonValue val : j_inputs)
    {
        QJsonObject obj = val.toObject();
        QString name = obj["name"].toString();
        QString type = obj["type"].toString();
        outputs[name] = new IOChannel(name, type);
    }
}

QJsonObject ModuleConfig::getJson()
{
    QJsonObject obj;
    obj["name"] = name;
    obj["parent"] = parent;

    QJsonArray j_inputs;
    QStringList key_input = inputs.keys();
    for (QString name : key_input)
        j_inputs << inputs[name]->getJson();

    QJsonArray j_outputs;
    QStringList key_output = outputs.keys();
    for (QString name : key_output)
        j_outputs << outputs[name]->getJson();

    obj["inputs"] = j_inputs;
    obj["outputs"] = j_outputs;
    return obj;
}
