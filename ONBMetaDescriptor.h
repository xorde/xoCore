#ifndef ONBMETADESCRIPTOR_H
#define ONBMETADESCRIPTOR_H

#include "ui/properties/AbstractMetaDescriptor.h"
#include "ONBMetaDescription.h"

class XOCORESHARED_EXPORT ONBMetaDescriptor : public AbstractMetaDescriptor
{
    Q_OBJECT
public:
    ONBMetaDescriptor(QObject* parent = nullptr);

    virtual void set(QObject *object) override;
    virtual QJsonObject saveToJson() override;
    virtual bool loadFromQVariantMap(const QVariantMap &map) override;

    void setClassName(QString name);

    virtual QString getClassName() override;

protected:
    QString m_objectName = "";

};

#endif // ONBMETADESCRIPTOR_H
