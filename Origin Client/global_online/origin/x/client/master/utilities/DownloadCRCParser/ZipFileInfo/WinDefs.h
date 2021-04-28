#ifndef WINDEFS_H
#define WINDEFS_H

#include <windows.h>

#ifndef MAX
#define MAX(a,b) ((a>b) ? a : b)
#endif
#ifndef MIN
#define MIN(a,b) ((a<b) ? a : b)
#endif

#ifndef _countof
#define _countof(arr) sizeof(arr)/sizeof(arr[0])
#endif

#include <stdint.h>

typedef unsigned long int u_long;

#define EBILOGERROR qDebug()
#define EBILOGDEBUG qDebug()
#define EBILOGWARNING qDebug()

#define CORE_ASSERT Q_ASSERT

#include <QDebug>

#endif // WINDEFS_H
