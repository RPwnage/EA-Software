#include "UserIdleWatcher.h"

#include <QTimer>

#include "services/debug/DebugService.h"
#include "ConnectedUserPresenceStack.h"

#ifdef ORIGIN_PC
#include <Windows.h>
#endif

namespace
{
    const int IdleCheckIntervalMs = 1 * 1000;
    const int IdleThresholdMs = 5 * 60 * 1000;
    
    // Queries the number of milliseconds the user has been idle for
    int64_t userIdleMilliseconds();

#if defined(ORIGIN_PC) 
    int64_t userIdleMilliseconds()
    {
        LASTINPUTINFO li;
        li.cbSize = sizeof(li);
        if (GetLastInputInfo(&li))
        {
            return (GetTickCount() - li.dwTime);
        }

        return 0;
    }
#elif defined(ORIGIN_MAC)
    #include <IOKit/IOKitLib.h>
    #include <CoreFoundation/CFNumber.h>

    int64_t userIdleMilliseconds()
    {
        int64_t idletime = 0;

        io_iterator_t iter = 0;
        if (IOServiceGetMatchingServices(kIOMasterPortDefault, IOServiceMatching("IOHIDSystem"), &iter) == KERN_SUCCESS)
        {
            io_registry_entry_t entry = IOIteratorNext(iter);
            if (entry)
            {
                CFMutableDictionaryRef dict = NULL;
                if (IORegistryEntryCreateCFProperties(entry, &dict, kCFAllocatorDefault, 0) == KERN_SUCCESS)
                {
                    CFNumberRef obj = (CFNumberRef) CFDictionaryGetValue(dict, CFSTR("HIDIdleTime"));
                    if (obj)
                    {
                        int64_t nanoseconds = 0;
                        if (CFNumberGetValue(obj, kCFNumberSInt64Type, &nanoseconds))
                        {
                            idletime = (nanoseconds / 1000000); // Convert from nanoseconds to milliseconds.
                        }
                    }
                    CFRelease(dict);
                }
                IOObjectRelease(entry);
            }
            IOObjectRelease(iter);
        }

        return idletime;
    }
#else
    int userIdleMilliseconds()
    {
        return 0;
    }
#endif
}


namespace Origin
{
namespace Engine
{
namespace Social
{
    UserIdleWatcher::UserIdleWatcher(ConnectedUserPresenceStack *presenceStack, QObject *parent) :
        QObject(parent),
        mPresenceStack(presenceStack)
    {
        // Set up our idle polling
        QTimer *checkTimer = new QTimer(this);
        checkTimer->setInterval(IdleCheckIntervalMs);

        ORIGIN_VERIFY_CONNECT(checkTimer, SIGNAL(timeout()), this, SLOT(checkIdle()));
        checkTimer->start();
    }
    
    void UserIdleWatcher::checkIdle()
    {
        if (userIdleMilliseconds() > IdleThresholdMs)
        {
            // We're idle!
            const Chat::Presence awayPresence(Chat::Presence::Away);
            mPresenceStack->setPresenceForPriority(ConnectedUserPresenceStack::AutoAwayPriority, awayPresence);
        }
        else
        {
            mPresenceStack->clearPresenceForPriority(ConnectedUserPresenceStack::AutoAwayPriority);
        }
    }
}
}
}
