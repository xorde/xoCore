#include "ScriptEngineWrapper.h"

ScriptEngineWrapper::ScriptEngineWrapper(QObject *parent) : QScriptEngine(parent)
{

}

void ScriptEngineWrapper::addVariable(QObject *variable)
{
    QString name = variable->property("name").toString();
    if (name.isEmpty())
        return;

    if (globalObject().property(name).isValid()) // if variable exists
        deleteVariable(variable);

    globalObject().setProperty(name, newQObject(variable));
}

void ScriptEngineWrapper::deleteVariable(QObject *variable)
{
    QString name = variable->property("name").toString();
    globalObject().setProperty(name, QScriptValue());
}

void ScriptEngineWrapper::renameVariable(QString oldName, QString newName)
{
    globalObject().setProperty(newName, globalObject().property(oldName));
    globalObject().setProperty(oldName, QScriptValue());
}
