#ifndef COMPONENTSCONFIGPARSER_H
#define COMPONENTSCONFIGPARSER_H

#include <QObject>
#include "xoCore_global.h"

class XOCORESHARED_EXPORT ComponentsConfigParser : public QObject
{
    Q_OBJECT
public:
    explicit ComponentsConfigParser(QString path, QObject *parent = nullptr);

    QStringList getComponentNames() { return componentNames; }

private:
    QStringList componentNames;
};

#endif // COMPONENTSCONFIGPARSER_H
