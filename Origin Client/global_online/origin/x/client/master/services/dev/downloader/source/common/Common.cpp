#include "Common.h"

#ifdef Q_OS_WIN
#include <windows.h> // for Sleep
#endif

#include <QFile>
#include <QString>
#include <QDebug>

namespace Origin
{
    const char * _stripNamespaces(const char * string)
    {
        const char * p = strstr(string, "::");
        while ( p )
        {
            string = p + 2;
            p = strstr(string, "::");
        }
        return string;
    }
}
