#ifndef COMPONENTCONNECTION_H
#define COMPONENTCONNECTION_H

#include <QObject>
#include <QJsonObject>
#include "xoCore_global.h"

class XOCORESHARED_EXPORT ComponentConnection : public QObject
{
Q_OBJECT
public:
    ComponentConnection(QString outputComponentName,
                     QString outputComponentType,
                     QString outputName,
                     QString outputType,
                     QString inputComponentName,
                     QString inputComponentType,
                     QString inputName,
                     QString inputType,
                     int RMIP);

    QString outputComponentName;
    QString outputComponentType;
    QString outputName;
    QString outputType;
    QString inputComponentName;
    QString inputComponentType;
    QString inputName;
    QString inputType;

    bool isEnabled = true;

    int RMIP; //рекомендуемый минимальный интервал передачи

    QString compoundString()
    {
        QString connStr = "";
        connStr += outputComponentName;
        connStr += ".";
        connStr += outputName;
        connStr += " -> ";
        connStr += inputComponentName;
        connStr += ".";
        connStr += inputName;
        return connStr;
    }

    QJsonObject toJsonObject();

    static ComponentConnection * fromJson(QJsonObject &object);
};


#endif // COMPONENTCONNECTION_H
