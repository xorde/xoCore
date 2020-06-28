#ifndef SCRIPTENGINEWRAPPER_H
#define SCRIPTENGINEWRAPPER_H

#include <QtScript/QScriptEngine>

#include "xoCore_global.h"

class XOCORESHARED_EXPORT ScriptEngineWrapper : public QScriptEngine
{
    Q_OBJECT
public:
    explicit ScriptEngineWrapper(QObject *parent = nullptr);

public slots:
    void addVariable(QObject * variable);
    void deleteVariable(QObject *variable);
    void renameVariable(QString oldName, QString newName);

};

#endif // SCRIPTENGINEWRAPPER_H
