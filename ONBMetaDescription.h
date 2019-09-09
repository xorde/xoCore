#ifndef ONBMETADESCRIPTION_H
#define ONBMETADESCRIPTION_H

#include "ui/properties/AbstractMetaDescription.h"
#include "Module/ComponentProxyONB.h"
#include "Protocol/ObjectInfo.h"

#include <QJsonObject>

class XOCORESHARED_EXPORT ONBMetaDescription : public AbstractMetaDescription
{
    Q_OBJECT
public:
    explicit ONBMetaDescription(ObjectProxy *object, QObject *parent = nullptr);

    virtual QMetaEnum enumerator() override;
    virtual bool isEnumType() override;
    QJsonObject toJsonObject();

public slots:
    virtual bool setValue(const QVariant &value) override;
    virtual const QVariant getValue() override;
    virtual void setDefault() override;
    virtual void valueChangedSlot() override;
    virtual bool setTextValue(QString value) override;

private:
    ObjectProxy* object = nullptr;

};

#endif // ONBMETADESCRIPTION_H
