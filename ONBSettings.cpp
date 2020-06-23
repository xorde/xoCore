#include "ONBSettings.h"

ONBSettings::ONBSettings(ComponentProxyONB *component, ONBChannelType channelType, QObject *parent) : AbstractSettings(parent)
{
    m_objectName = component->componentName();

    switch (channelType)
    {
        case Inputs: channels = component->getInputs(); break;
        case Outputs: channels = component->getOutputs(); m_isReadOnly = true; break;
        case Settings: channels = component->getSettings(); break;
    }

    for(auto channel : channels)
        connect(channel, &ObjectProxy::valueChanged, this, [=]() { emit propertyChanged(channel->name()); });
}

AbstractMetaDescriptor *ONBSettings::metaDescriptor()
{
    if (!m_metaDescriptor)
    {
        m_metaDescriptor = new ONBMetaDescriptor();
        m_metaDescriptor->set(this);
        m_metaDescriptor->setClassName(m_objectName);
    }

    m_metaDescriptor->setReadOnly(m_isReadOnly);

    return m_metaDescriptor;
}

QString ONBSettings::name() { return m_objectName; }

bool ONBSettings::getIsReadOnly() { return m_isReadOnly; }
