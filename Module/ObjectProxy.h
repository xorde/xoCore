#ifndef OBJECTPROXY_H
#define OBJECTPROXY_H

#include "Protocol/ObjectInfo.h"
#include <QJsonObject>
#include <QVariant>
#include <QObject>
#include <QTimer>
#include "xoCore_global.h"

class ComponentProxyONB;

class XOCORESHARED_EXPORT ObjectProxy : public QObject, public ObjectInfo
{
    Q_OBJECT

public:
    ObjectProxy(ComponentProxyONB *component, const ObjectDescription &desc);
    ~ObjectProxy() override;

    bool isValid();

    QString readTypeString() const;
    QString writeTypeString() const;

    QJsonObject getDescriptionJSON() const;

    QVariant value() const;
    void setValue(QVariant v);

    QVariant min() const {return mMin;}
    QVariant max() const {return mMax;}
    QVariant def() const {return mDef;}
    QVariant step() const {return mStep;}
    QString mimeType() const {return m_mimeType;}
    QString hint() const {return m_hint;}
    QString unit() const {return m_unit;}
    QString options() const {return m_options;}
    QStringList enumList() const
    {
        return m_enum;
    }

//    bool link(ObjectProxy *other);
//    bool unlink(); // TODO
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
    virtual bool writeMeta(const QByteArray &ba, MetaValue id) override;
    void receiveEvent() override {emit received();}
    void valueChangeEvent() override {emit valueChanged();}

    void setDescription(const ObjectDescription &desc);

private:
    ComponentProxyONB *mComponent;
    QVariant mMin, mMax, mDef, mStep;
    QString m_mimeType, m_hint, m_unit, m_options;
    QStringList m_enum;
    QTimer *mAutoRequestTimer = nullptr;
    const void *mDefaultReadPtr = nullptr;
    ObjectProxy *mLinkedPublisher = nullptr;
    unsigned short m_extFlagsReceived = 0;

    static QString nameOfType(Type type)
    {
        return QVariant::typeToName(type);
    }

    QVariant toVariant(const void *ptr) const;
    bool fromVariant(QVariant v, void *ptr);

    QVariant toVariant(const QByteArray &ba) const;

    friend class ComponentProxyONB;

    void *_allocate(Type t, unsigned short sz);
    void _free(void *ptr, Type t, unsigned short sz);
    void allocateBuffers();
    void destroyBuffers();
};

#endif // OBJECTPROXY_H
