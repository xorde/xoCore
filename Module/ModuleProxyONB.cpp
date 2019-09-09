#include "ModuleProxyONB.h"

ModuleProxyONB::ModuleProxyONB(QString module_name, QObject *parent) :
    QObject(parent),
    m_name(module_name)
{
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &ModuleProxyONB::enumerateComponents);
    timer->start(200);
}

ComponentProxyONB *ModuleProxyONB::component(unsigned short compID) const
{
    return m_components.value(compID, nullptr);
}

ComponentProxyONB *ModuleProxyONB::component(QString name) const
{
    return m_componentMap.value(name, nullptr);
}

ComponentProxyONB *ModuleProxyONB::classInfo(uint32_t cid) const
{
    return m_classInfo.value(cid, nullptr);
}

ComponentProxyONB *ModuleProxyONB::classInfo(QString name) const
{
    return m_classInfo.value(m_classMap.value(name, 0UL), nullptr);
}

const QList<uint32_t> &ModuleProxyONB::classes() const
{
    return m_classes;
}

QStringList ModuleProxyONB::classNames() const
{
    return m_classMap.keys();
}

QStringList ModuleProxyONB::componentNames() const
{
    return m_componentMap.keys();
}

QStringList ModuleProxyONB::componentNames(uint32_t classID) const
{
    QStringList result;
    for (ComponentProxyONB *c: m_components)
        if (c->classId() == classID)
            result << c->componentName();
    return result;
}

QStringList ModuleProxyONB::componentNames(QString className) const
{
    QStringList result;
    for (ComponentProxyONB *c: m_componentMap)
        if (c->componentType() == className)
            result << c->componentName();
    return result;
}

QList<ModuleConfig*> ModuleProxyONB::getModuleConfig(QString in_my_name, bool inInstances)
{
    QList<ModuleConfig*> list;

    QList<ComponentProxyONB *> compList = inInstances ? m_componentMap.values() : m_classInfo.values();

    foreach (ComponentProxyONB *component, compList)
    {        
        ModuleConfig* compConf = new ModuleConfig();
        compConf->name = component->componentType();
        compConf->parent = in_my_name.isEmpty() ? name() : in_my_name;

        for (auto io : component->getOutputs())
            compConf->outputs[io->name()] = new IOChannel(io->name(), io->writeTypeString());

        for (auto io : component->getInputs())
            compConf->inputs[io->name()] = new IOChannel(io->name(), io->readTypeString());

        list.push_back(compConf);
    }
    return list;
}


void ModuleProxyONB::receiveData(const QByteArray &data)
{
    receivePacket(ONBPacket(data));
}

void ModuleProxyONB::receivePacket(const ONBPacket &packet)
{
    parsePacket(packet);
    unsigned short compID = packet.header().componentID;
    if (packet.header().classInfo)
        parseClassInfo(packet);
    else if (m_components.contains(compID))
        m_components[compID]->receiveData(packet);
}

void ModuleProxyONB::enumerateClasses()
{
    ONBHeader hdr;
    hdr.objectID = aidPollClasses;
    hdr.componentID = 0;
    hdr.svc = 1;
    hdr.classInfo = 1;
    ONBPacket packet(hdr);
    sendPacket(packet);
}

void ModuleProxyONB::enumerateComponents()
{
    ONBHeader hdr;
    hdr.objectID = aidPollNodes;
    hdr.svc = 1;
    ONBPacket packet(hdr);
    sendPacket(packet);
}

void ModuleProxyONB::requestIcon()
{
    ONBHeader hdr;
    hdr.objectID = svcIcon;
    hdr.componentID = 0;
    hdr.svc = 1;
    hdr.local = 1;
    sendPacket(ONBPacket(hdr));
}

void ModuleProxyONB::assignComponentID(unsigned short compID)
{
    ONBHeader hdr;
    hdr.objectID = svcWelcome;
    hdr.componentID = 0;
    hdr.svc = 1;
    hdr.local = 1;
    QByteArray data(reinterpret_cast<const char*>(&compID), sizeof(unsigned short));
    ONBPacket packet(hdr, data);
    sendPacket(packet);

    if (!m_components.contains(compID))
    {
        ComponentProxyONB *c = new ComponentProxyONB(compID, this);
        connect(c, SIGNAL(newData(ONBPacket)), SLOT(sendPacket(ONBPacket)));
        connect(c, SIGNAL(ready()), SLOT(componentReady()));
        connect(c, SIGNAL(infoChanged()), SLOT(componentInfoChanged()));
        m_components[compID] = c;
        c->requestInfo();
        emit componentAdded(compID);
    }
}
//---------------------------------------------------------

bool ModuleProxyONB::createComponent(uint32_t classID, QString name)
{
    if (!m_classes.contains(classID))
        return false;
    if (!m_classInfo[classID]->isFactory())
        return false;

    ONBHeader hdr;
    hdr.objectID = svcCreate;
    hdr.componentID = 0;
    hdr.svc = 1;
    hdr.local = 1;
    QByteArray data(reinterpret_cast<const char*>(&classID), sizeof(uint32_t));
    if (!name.isEmpty())
        data.append(name.toUtf8());
    ONBPacket packet(hdr, data);
    sendPacket(packet);
    return true;
}

bool ModuleProxyONB::createComponent(QString className)
{
    return createComponent(m_classMap.value(className, 0UL));
}

bool ModuleProxyONB::createComponent(QString className, QString componentName)
{
    return createComponent(m_classMap.value(className, 0UL), componentName);
}

bool ModuleProxyONB::deleteComponent(unsigned short compID)
{
    ONBHeader hdr;
    hdr.objectID = svcKill;
    hdr.componentID = 0;
    hdr.svc = 1;
    hdr.local = 1;
    QByteArray data(reinterpret_cast<const char*>(&compID), sizeof(unsigned short));
    ONBPacket packet(hdr, data);
    sendPacket(packet);
    return m_components.contains(compID);
}

bool ModuleProxyONB::deleteComponent(QString name)
{
    if (m_componentMap.contains(name))
        return deleteComponent(m_componentMap[name]->id());
    return false;
}

bool ModuleProxyONB::renameComponent(QString name, QString newName)
{
    if (m_componentMap.contains(name))
    {
        if (m_componentMap.contains(newName))
            return false;
        m_componentMap[name]->setComponentName(newName);
        return true;
    }
    return false;
}

void ModuleProxyONB::sendPacket(const ONBPacket &packet)
{
    emit newPacket(packet);
    QByteArray ba;
    packet.writePacket(ba);
    newDataToSend(ba);
}

void ModuleProxyONB::parsePacket(const ONBPacket &packet)
{
    if (!packet.header().svc)
        return;

    unsigned char oid = packet.header().objectID;
    unsigned short compID = packet.header().componentID;

    switch(oid)
    {
      case svcIcon:
        if (!compID)
            m_iconData = packet.data();
        break;

      case svcEcho:
//        break;
      case svcHello:
        if (!compID)
            emit newComponent();
        else
        {

        }
        break;

      case svcKill:
        if (m_components.contains(compID))
        {
            emit componentKilled(compID);
            m_componentMap.remove(m_components[compID]->componentName());
            m_components.remove(compID);
        }
        break;

      case svcMessage:
        if (m_components.contains(compID))
            emit message(QString::fromUtf8(packet.data()), m_components[compID]->componentName());
        else
            emit message(QString::fromUtf8(packet.data()));
        break;
    }
}

void ModuleProxyONB::sendClassInfoPacket(const ONBPacket &packet)
{
    ONBHeader hdr = packet.header();
    hdr.classInfo = 1;
    ONBPacket pkt(hdr, packet.data());
    sendPacket(pkt);
}

void ModuleProxyONB::parseClassInfo(const ONBPacket &packet)
{
    const ONBHeader &header = packet.header();
//    qDebug() << "class info" << header.componentID << header.objectID;

    if (!header.componentID)
    {
        // if module contains no classes => it's ready
        if (header.objectID == svcClass && m_classes.isEmpty())
            emit ready();
        return;
    }

    int idx = header.componentID - 1;
    if (idx == m_classes.size())
    {
        if (header.objectID == svcClass)
        {
            uint32_t cid = *reinterpret_cast<const uint32_t*>(packet.data().data());
            m_classes << cid;
            ComponentProxyONB *c = new ComponentProxyONB(header.componentID, this);
            connect(c, SIGNAL(newData(ONBPacket)), SLOT(sendClassInfoPacket(ONBPacket)));
            m_classInfo[cid] = c;
            // request class info
            ONBHeader hdr;
            hdr.objectID = svcRequestAllInfo;
            hdr.componentID = c->id();
            hdr.classInfo = 1;
            hdr.svc = 1;
            hdr.local = 1;
            QByteArray data(reinterpret_cast<const char*>(&cid), 4);
            sendPacket(ONBPacket(hdr, data));
        }
        else
        {
            qDebug() << "[ModuleProxyONB] unknown class info received";
        }
    }
    else if (idx < m_classes.size())
    {
        uint32_t cid = m_classes[idx];
        ComponentProxyONB *c = m_classInfo[cid];
        c->receiveData(packet);
        if (c->isReady())
        {
            if (c->m_className.isEmpty())
                c->m_className = c->componentName();
            m_classMap[c->m_className] = cid;
        }
    }
    else
    {
        qDebug() << "[ModuleProxyONB] unknown class info received";
    }

//    qDebug() << name();

    bool readyFlag = true;
    for (auto &clinfo: m_classInfo)
    {
//        qDebug() << "    " << clinfo->componentType() << clinfo->isReady();

        if (!clinfo->isReady())
            readyFlag = false;
    }
    if (readyFlag)
    {
        emit ready();
    }
}

void ModuleProxyONB::componentReady()
{
    ComponentProxyONB *comp = qobject_cast<ComponentProxyONB*>(sender());

    uint32_t cid = comp->classId();
    if (!m_classes.contains(comp->classId()))
    {
        m_classes << cid;
        ComponentProxyONB *proto = new ComponentProxyONB(m_classes.size(), this);
        comp->extractPrototype(proto);
        m_classMap[proto->componentType()] = cid;
        m_classInfo[cid] = proto;
        m_classInfo[cid]->m_isFactory = false;
        emit ready();
    }

    ComponentProxyONB *proto = m_classInfo[cid];
    if (comp->m_className.isEmpty())
        comp->m_className = proto->m_className;

    if (!proto->isFactory())
        comp->setComponentName(comp->componentType() + QString().sprintf("_%08X", comp->serialNumber()));

    if (comp)
        m_componentMap[comp->componentName()] = comp;
}

void ModuleProxyONB::componentInfoChanged()
{
    ComponentProxyONB *comp = qobject_cast<ComponentProxyONB*>(sender());
    if (comp)
    {
        if (m_componentMap.value(comp->componentName(), nullptr) != comp) // component was renamed
        {
            QString oldName = m_componentMap.key(comp);
            m_componentMap.remove(oldName);
            m_componentMap[comp->componentName()] = comp;
        }
    }
}
