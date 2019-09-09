#ifndef MODULECONFIG_H
#define MODULECONFIG_H

#include <QList>
#include <QJsonArray>
#include <QJsonObject>
#include "xoCore_global.h"

struct IOChannel
{
    IOChannel(QString name = "", QString type = ""):name(name), type(type){}
    QString name;
    QString type;

    QJsonObject getJson()
    {
        QJsonObject obj;
        obj["name"] = name;
        obj["type"] = type;
        return obj;
    }
};
Q_DECLARE_METATYPE(IOChannel)

class XOCORESHARED_EXPORT ModuleConfig
{
public:
    enum Type
    {
        MODULE,
        PLUGIN
    };

    ModuleConfig();
    ModuleConfig(const QJsonObject& in_obj);
    ~ModuleConfig();

    QJsonObject getJson();
    void parseJson(QJsonObject in_obj);

    QString name;
    QString parent;

    QHash<QString, IOChannel*> inputs;
    QHash<QString, IOChannel*> outputs;

    Type type;
};

#endif // MODULECONFIG_H
