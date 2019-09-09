#include "ComponentConnection.h"



ComponentConnection::ComponentConnection(QString outputComponentName, QString outputModuleType, QString outputName, QString outputType, QString inputModuleName, QString inputModuleType, QString inputName, QString inputType, int RMIP) :
    outputComponentName(outputComponentName),
    outputComponentType(outputModuleType),
    outputName(outputName),
    outputType(outputType),
    inputComponentName(inputModuleName),
    inputComponentType(inputModuleType),
    inputName(inputName),
    inputType(inputType),
    RMIP(RMIP)
{

}


ComponentConnection *ComponentConnection::fromJson(QJsonObject &object)
{
    QString outputModuleName = object.value("outputModuleName").toString();
    QString outputModuleType = object.value("outputModuleType").toString();
    QString outputName = object.value("outputName").toString();
    QString outputType = object.value("outputType").toString();
    QString inputModuleName = object.value("inputModuleName").toString();
    QString inputModuleType = object.value("inputModuleType").toString();
    QString inputName = object.value("inputName").toString();
    QString inputType = object.value("inputType").toString();
    int RMIP = object.value("RMIP").toInt();

    //TODO: consistency check

    auto connection = new ComponentConnection(outputModuleName,
                                              outputModuleType,
                                              outputName,
                                              outputType,
                                              inputModuleName,
                                              inputModuleType,
                                              inputName,
                                              inputType,
                                              RMIP);

    return connection;
}

QJsonObject ComponentConnection::toJsonObject()
{
    QJsonObject connectionObject;
    connectionObject.insert("outputModuleName", outputComponentName);
    connectionObject.insert("outputModuleType", outputComponentType);
    connectionObject.insert("outputName", outputName);
    connectionObject.insert("outputType", outputType);
    connectionObject.insert("inputModuleName", inputComponentName);
    connectionObject.insert("inputModuleType", inputComponentType);
    connectionObject.insert("inputName", inputName);
    connectionObject.insert("inputType", inputType);
    connectionObject.insert("RMIP", RMIP);
    return connectionObject;
}
