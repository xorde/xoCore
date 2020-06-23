#ifndef ONBSETTINGS_H
#define ONBSETTINGS_H

#include "AbstractSettings.h"
#include "ONBMetaDescriptor.h"
#include "Module/ComponentProxyONB.h"
#include "helpers/ConnectionHelper.h"
#include "xoCore_global.h"

class XOCORESHARED_EXPORT ONBSettings : public AbstractSettings
{
    Q_OBJECT
public:

    explicit ONBSettings(ComponentProxyONB *component, ONBChannelType channelType = ONBChannelType::Settings, QObject *parent = nullptr);

    virtual AbstractMetaDescriptor *metaDescriptor() override;
    QList<ObjectProxy *> channels;

    QString name();

    bool getIsReadOnly();

private:

    bool m_isReadOnly = false;

    ONBMetaDescriptor* m_metaDescriptor = nullptr;
    QString m_objectName = "";

};

#endif // ONBSETTINGS_H
