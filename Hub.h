#ifndef HUB_H
#define HUB_H

#include <QObject>
#include <queue>
#include <QFuture>
#include <QFutureWatcher>

#include "Scheme.h"

#include "Module/ModuleProxyONB.h"
#include "Module/ComponentProxyONB.h"

#include "helpers/ConnectionHelper.h"

#include "thread_safe/thread_safe_queue.h"
#include "ComponentBase.h"
#include "xoCore_global.h"

using namespace std;

struct String4
{
    String4(QString _a = "", QString _b = "", QString _c = "", QString _d = "")
    {
        a = _a;
        b = _b;
        c = _c;
        d = _d;
    }
    QString a = "";
    QString b = "";
    QString c = "";
    QString d = "";

    bool equals(String4 &other, Qt::CaseSensitivity caseSense = Qt::CaseInsensitive)
    {
        if (a.compare(other.a, caseSense) != 0) return false;
        if (b.compare(other.b, caseSense) != 0) return false;
        if (c.compare(other.c, caseSense) != 0) return false;
        if (d.compare(other.d, caseSense) != 0) return false;
        return true;
    }
};

class XOCORESHARED_EXPORT Hub : public QObject
{
    Q_OBJECT
public:
    Hub(QObject* parent = nullptr);
    ~Hub();

    void setScheme(Scheme* m_scheme);
    Scheme* getScheme();
    void addModule(ModuleProxyONB *connection);
    void removeModule(int moduleId);

    void setIsEnabled(bool enabled);
    void linkConnection(ComponentConnection* connection, bool shouldConnect);
    bool isEnabled();

    QList<ModuleProxyONB*> getModules();
    ModuleProxyONB *getModuleByName(const QString &name);
    ComponentProxyONB* getComponentByName(QString name);
    void clearConnections();

public slots:
    void checkCurrentSchemeComponents();

signals:
    void moduleReady(ModuleProxyONB *module);
    void componentAdded(QObject *obj);
    void componentKilled(QObject *obj);
    void componentChanged(QObject *obj);
    void componentRenamed(QString oldName, QString newName);
    void schemeChanged(Scheme* scheme);
    void enableChanged(bool enabled);

protected:
    void reloadComponentSettingsFromScheme(QString compName);
    void reloadComponentSettingsFromScheme(ComponentInfo *compInfo);

    ConnectionHelper m_schemeConnections;
    ConnectionHelper m_moduleConnections;
    ConnectionHelper m_pluginConnections;

    Scheme* m_scheme = nullptr;

    bool m_isEnabled = false;

    QHash<int, ModuleProxyONB*> modulesById;
    QHash<QString, ComponentProxyONB*> componentsByName;
    QSet<QString> componentConnections;

    QList<QMetaObject::Connection> connections;
};

#endif // HUB_H
