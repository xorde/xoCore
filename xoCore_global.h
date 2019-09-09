#ifndef XOCORE_GLOBAL_H
#define XOCORE_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(XOCORE_LIBRARY)
#  define XOCORESHARED_EXPORT Q_DECL_EXPORT
#else
#  define XOCORESHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // XOCORE_GLOBAL_H
