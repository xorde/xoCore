#ifndef XOCOREPLUGIN_H
#define XOCOREPLUGIN_H

#include <QObject>
#include "xoCore_global.h"

class XOCORESHARED_EXPORT xoCorePlugin : public QObject
{
Q_OBJECT
public:
    explicit xoCorePlugin(QObject *parent = nullptr) : QObject (parent)
    {

    }
    virtual void start() = 0;
    virtual bool providesUI() = 0;
};



#endif // XOCOREPLUGIN_H
