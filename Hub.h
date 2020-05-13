#ifndef HUB_H
#define HUB_H

#include <queue>
#include <QObject>
#include <QFuture>
#include <QFutureWatcher>

#include "Scheme.h"
#include "ComponentBase.h"

#include "Module/ModuleProxyONB.h"
#include "Module/ComponentProxyONB.h"
#include "helpers/ConnectionHelper.h"

#include "thread_safe/thread_safe_queue.h"
#include "xoCore_global.h"

using namespace std;

struct String3
{
    String3(QString _componentName = "", QString _componentType = "", QString _moduleName = "")
    {
        componentName = _componentName;
        componentType = _componentType;
        moduleName = _moduleName;
    }

    QString componentName;
    QString componentType;
    QString moduleName;

    bool equals(String3 &other, Qt::CaseSensitivity caseSense = Qt::CaseInsensitive)
    {
        if (componentName.compare(other.componentName, caseSense) != 0) return false;
        if (componentType.compare(other.componentType, caseSense) != 0) return false;
        if (moduleName.compare(other.moduleName, caseSense) != 0) return false;
        return true;
    }
};

class XOCORESHARED_EXPORT Hub : public QObject
{
    Q_OBJECT
public:
    Hub(QObject* parent = nullptr);

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
};

#endif // HUB_H
