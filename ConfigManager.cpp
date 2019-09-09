#include "ConfigManager.h"

#include <QDir>
#include <QFile>
#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>

#include "Core.h"
#include "fileutilities.h"

void ConfigManager::writeModuleConfig(ModuleProxyONB *module)
{
    if(!module) return;

    QString path = Core::FolderConfigs + module->name() + "/";
    verifyDir(module->name());

    auto image = module->icon();


    if(!image.isNull())
    {
        image.convertToFormat(QImage::Format_RGBA8888);

        if (image.size() != QSize(24, 24))
        {
            image = image.scaled(24, 24, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        }
        image.save(path + module->name() + ".png");
    }

    for(auto& name : module->classNames())
    {
        writeComponentConfig(module, module->classInfo(name));
    }
}

void ConfigManager::writeComponentConfig(ModuleProxyONB* module, ComponentProxyONB *component)
{
    QElapsedTimer t; t.restart();

    QString path = Core::FolderConfigs + module->name() + "/";
    verifyDir(module->name());

    QString filepath = path + component->componentType() + Core::FileExtensionConfigDot;
    QFile file(filepath);

    if(!file.exists())
    {
        if(file.open(QIODevice::WriteOnly|QIODevice::Truncate))
        {
            file.write(QJsonDocument(component->getInfoJson()).toJson(QJsonDocument::Compact));
            file.close();
        }

        QImage image;
        if(!component->icon().isNull())
          image = component->icon();



        if(!image.isNull())
        {
            image.convertToFormat(QImage::Format_RGBA8888);
            if (image.size() != QSize(24, 24))
            {
                image = image.scaled(24, 24, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            }
            image.save(path + module->name() + "_" + component->componentType() + ".png");
        }

        qDebug() << "Config for component written" << component->componentType() << t.elapsed();
    }
    else
    {
        qDebug() << "Config exists" << component->componentType() << t.elapsed();
    }


}

ConfigManager::ConfigManager()
{
    m_path_full = Core::FolderConfigs;
}

bool ConfigManager::moduleConfigsExist(QString moduleName)
{
    return QDir(Core::FolderConfigs + moduleName + "/").exists();
}

QJsonObject ConfigManager::readConfigurations()
{
    QString configsPath = Core::FolderConfigs;

    QJsonObject result;

    for(auto& moduleName : QDir(configsPath).entryList(QDir::Dirs | QDir::NoDotAndDotDot))
    {
        result[moduleName] = ConfigManager::readConfiguration(configsPath + moduleName);
    }

    return result;
}

QByteArray ConfigManager::jsonToByteArray(const QJsonObject &in_obj)
{
    QJsonDocument jDoc(in_obj);
    return jDoc.toJson(QJsonDocument::Compact);
}

void ConfigManager::verifyDir(QString moduleName)
{
    FileUtilities::createIfNotExists(Core::FolderConfigs + moduleName + "/");
}

QJsonObject ConfigManager::toJson(const QByteArray& in_content)
{
    QJsonDocument doc = QJsonDocument::fromJson(in_content);
    QJsonObject obj = doc.object();
    return obj;
}


QJsonObject ConfigManager::readConfiguration(QString in_path)
{
    QJsonObject object;
    for (QString file_name : QDir(in_path).entryList(QDir::Files, QDir::NoSort))
    {
        QFile file(in_path + "/" + file_name);
        if (file.open(QIODevice::ReadOnly))
        {
            QByteArray data = file.readAll();
            QJsonObject inner = QJsonDocument::fromJson(data).object();
            object[inner["type"].toString()] = inner;
        }
    }

    return object;
}


void ConfigManager::writeToFile(const QJsonObject& in_obj, QString in_path)
{
    QStringList keys = in_obj.keys();
    for (QString key : keys)
    {
        QJsonObject obj = in_obj[key].toObject();

        QString file_name = obj["className"].toString();
        QFile file(in_path + "/" + file_name);

        QDir dir(in_path);
        if(!dir.exists()) dir.mkdir(in_path);

        if (file.open(QIODevice::WriteOnly))
            file.write(jsonToByteArray(obj));
    }
}
