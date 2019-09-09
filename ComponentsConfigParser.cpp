#include "ComponentsConfigParser.h"

#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QDir>


ComponentsConfigParser::ComponentsConfigParser(QString path, QObject *parent) : QObject(parent)
{
    componentNames.clear();

    QDir pluginsDir(path + "/Plugins");
    QDir proxiesDir(path + "/Proxies");

    auto queryDirectory = [=](QDir dir)
    {
        auto configInfos = dir.entryInfoList(QDir::Files);
        for(auto& info : configInfos)
        {
            QFile file(info.absoluteFilePath());
            if(!file.open(QIODevice::ReadOnly)) { qDebug() << "Невозможно открыть файл конфига компонентов!"; return; }

            auto document = QJsonDocument::fromJson(file.readAll());
            componentNames << document.object()["name"].toString();
        }
    };

    queryDirectory(pluginsDir);
    queryDirectory(proxiesDir);
}
