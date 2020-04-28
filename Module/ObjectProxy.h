#ifndef OBJECTPROXY_H
#define OBJECTPROXY_H

#include "Protocol/xoObjectBase.h"
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
    void send();

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
    QTimer *mAutoRequestTimer = nullptr;
    ObjectProxy *mLinkedPublisher = nullptr;

    QVariant toVariant(const QByteArray &ba) const;

    friend class ComponentProxyONB;
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

        T newValue = v.value<T>();
        m_changed = (*this->m_ptr != newValue);
        *this->m_ptr = newValue;

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
            //! @warning This is dangerous!! or not
            this->m_ptr = reinterpret_cast<const ObjectProxyImpl<T>*>(publisher)->m_ptr;
            ObjectBase::m_description.size = publisher->description().size;
            return true;
        }
        return false;
    }

    virtual void unlink() override
    {
        this->m_ptr = &this->m_value;
        restoreDescription();
    }
};

#endif // OBJECTPROXY_H
