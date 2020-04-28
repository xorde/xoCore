#ifndef OBJECTPROXY_H
#define OBJECTPROXY_H

#include "Protocol/ObjectInfo.h"
#include <QJsonObject>
#include <QVariant>
#include <QObject>
#include <QTimer>
#include "xoCore_global.h"

class ComponentProxyONB;

class XOCORESHARED_EXPORT ObjectProxy : public QObject, virtual public ObjectBase
{
    Q_OBJECT

public:
//    ObjectProxy(ComponentProxyONB *component, const ObjectDescription &desc);
    virtual ~ObjectProxy() override;

    bool isValid();

    QJsonObject getDescriptionJSON() const;

    virtual QVariant value() const = 0;
    virtual bool setValue(QVariant v) = 0;

    QVariant min() const {return testExtFlag(MV_Min)? getMeta(MV_Min): QVariant();}
    QVariant max() const {return testExtFlag(MV_Max)? getMeta(MV_Max): QVariant();}
    QVariant def() const {return testExtFlag(MV_Def)? getMeta(MV_Def): QVariant();}
    QVariant step() const {return testExtFlag(MV_Step)? getMeta(MV_Step): QVariant();}
    QString mimeType() const {return m_mimeType;}
    QString hint() const {return m_hint;}
    QString unit() const {return m_unit;}
    QString options() const {return m_options;}
    QStringList enumList() const
    {
        return m_enum;
    }

    static bool link(ObjectProxy *publisher, ObjectProxy *subscriber);
    static bool unlink(ObjectProxy *publisher, ObjectProxy *subscriber);

    void subscribe(int period_ms = -1);
    void unsubscribe();

public slots:
    void request() const;
    void send() const;
    void sendTimed();

signals:
    void valueChanged();
    void received();

protected:
    ObjectProxy(ComponentProxyONB *component, const ObjectDescription &desc);
    void receiveEvent() override {emit received();}
    void changeEvent() override {emit valueChanged();}

    //! @todo: recreate ObjectProxy for the given id instead of this:
//    void setDescription(const ObjectDescription &desc);

    virtual bool linkTo(const ObjectProxy *publisher) = 0;
    virtual void unlink() = 0;

    virtual QVariant getMeta(MetaValue) const {return QVariant();}

private:
    ComponentProxyONB *mComponent;
//    QVariant mMin, mMax, mDef, mStep;
//    QString m_mimeType, m_hint, m_unit, m_options;
//    QStringList m_enum;
    QTimer *mAutoRequestTimer = nullptr;
//    const void *mDefaultReadPtr = nullptr;
    ObjectProxy *mLinkedPublisher = nullptr;
//    unsigned short m_extFlagsReceived = 0;

    QVariant toVariant(const QByteArray &ba) const;

    friend class ComponentProxyONB;

//    void *_allocate(Type t, unsigned short sz);
//    void _free(void *ptr, Type t, unsigned short sz);
//    void allocateBuffers();
//    void destroyBuffers();
};

template <typename T>
class ObjectProxyImpl : public ObjectProxy, public xoObjectBase<T>
{
public:
    ObjectProxyImpl(ComponentProxyONB *component, const ObjectDescription &desc) :
        ObjectProxy(component, desc)
    {}

    virtual QVariant value() const override
    {
        return *this->m_ptr; // this must be equal to the line below but it's simpler
//        return QVariant::fromValue<T>(*this->m_ptr);
    }
    virtual bool setValue(QVariant v) override
    {
        if (!v.canConvert(this->m_description.type))
            return false;
        *this->m_ptr = v.value<T>();
        if (m_changed)
            send();
        return true;
    }

    virtual QVariant getMeta(MetaValue meta) const
    {
        switch (meta)
        {
            case MV_Min: return this->m_min;
            case MV_Max: return this->m_max;
            case MV_Def: return this->m_def;
            case MV_Step: return this->m_step;
            default: return QVariant();
        }
    }

protected:
#pragma warning (disable: 4250)
//    using ObjectBase::readMeta;

    virtual bool linkTo(const ObjectProxy *publisher) override
    {
        if (publisher->type() == type())
        {
            this->m_ptr = dynamic_cast<const ObjectProxyImpl<T>*>(publisher)->m_ptr;
            return true;
        }
        return false;
    }

    virtual void unlink() override
    {
        this->m_ptr = &this->m_value;
    }
};

#endif // OBJECTPROXY_H
