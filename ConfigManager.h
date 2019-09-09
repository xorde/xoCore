#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QJsonObject>

#include "Module/ModuleProxyONB.h"

class XOCORESHARED_EXPORT ConfigManager : public QObject
{
    Q_OBJECT
public:
    static void writeModuleConfig(ModuleProxyONB *module);
    static void writeComponentConfig(ModuleProxyONB *module, ComponentProxyONB* component);
    static bool moduleConfigsExist(QString moduleName);
    static QJsonObject readConfigurations();
    static QJsonObject readConfiguration(QString in_path);

protected:
    QString m_path_full;

    void writeToFile(const QJsonObject &in_obj, QString in_path);

    QJsonObject toJson(const QByteArray& in_content);
    QByteArray jsonToByteArray(const QJsonObject &in_obj);

private:
    static void verifyDir(QString moduleName);
    ConfigManager();
};

#endif // CONFIGMANAGER_H
