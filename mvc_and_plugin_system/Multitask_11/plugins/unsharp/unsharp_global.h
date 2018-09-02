#ifndef UNSHARP_GLOBAL_H
#define UNSHARP_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(UNSHARP_LIBRARY)
#  define UNSHARPSHARED_EXPORT Q_DECL_EXPORT
#else
#  define UNSHARPSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // UNSHARP_GLOBAL_H
