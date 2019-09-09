#ifndef COMPONENTINFO_H
#define COMPONENTINFO_H

#include <QObject>
#include <QJsonObject>
#include "ONBSettings.h"
#include <QJsonArray>
#include "xoCore_global.h"

class XOCORESHARED_EXPORT ComponentInfo : public QObject
{
Q_OBJECT
public:
    explicit ComponentInfo(QObject *parent = nullptr);

    QString type = "";
    QString name = "";
    QString parentModule = "";

    bool enabled = true;

    int visualX = 0;
    int visualY = 0;
//    int visualWidth = 100;
//    int visualHeight = 20;

    QMap<QString, QString> inputsWithType;
    QMap<QString, QString> outputsWithType;

    QJsonObject settings;

    QJsonObject toJsonObject();

    static QJsonArray jsonArrayFromStringMap(QMap<QString, QString> map);

    static QMap<QString, QString> stringMapFromJsonArray(QJsonArray array);

    static ComponentInfo *fromJson(QJsonObject &jsonObject);
};

#endif // COMPONENTINFO_H
