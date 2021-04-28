#include "CapsLockProxy.h"

#ifdef Q_OS_WIN
#include <windows.h>
#elif defined Q_OS_MAC
#include <Carbon/Carbon.h>
#endif

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {
            bool CapsLockProxy::capsLockActive()
            {
#ifdef Q_OS_WIN
                return GetKeyState(VK_CAPITAL) & 1;
#elif defined Q_OS_MAC
                return GetCurrentKeyModifiers() & alphaLock;
#endif
            }
        }
    }
}