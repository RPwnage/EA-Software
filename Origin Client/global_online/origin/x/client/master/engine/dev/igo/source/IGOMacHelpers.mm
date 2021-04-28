#include "IGOMacHelpers.h"

// Temporary mac until the build system is fixed
#ifndef ORIGIN_MAC
#define ORIGIN_MAC
#endif

#import <Cocoa/Cocoa.h>

#include "MacDllMain.h"
#include "IGOIPC/IGOIPC.h"

namespace Origin
{
namespace Engine
{
        
void SetupIGOWindow(void* winId, float alphaValue)
{
    NSView* view = (NSView*)winId;
    
    NSWindow* window = [view window];
    
    [window setIgnoresMouseEvents:YES];
    [window setBackgroundColor:[NSColor clearColor]];
    [window setOpaque:NO];
    [window setAlphaValue:alphaValue];
    [window setCollectionBehavior:NSWindowCollectionBehaviorTransient | NSWindowCollectionBehaviorIgnoresCycle];
}

void ForwardKeyData(void* winId, const char* data, int dataSize)
{
    NSView* view = (NSView*)winId;
    
    CFDataRef eventData = CFDataCreateWithBytesNoCopy(NULL, (UInt8*)data, CFIndex(dataSize), kCFAllocatorNull);
    CGEventRef event = CGEventCreateFromData(NULL, eventData);
    
    NSEvent* wndEvent = [NSEvent eventWithCGEvent:event];
    [[view window] postEvent:wndEvent atStart:NO];
    
    CFRelease(event);
    CFRelease(eventData);
}

bool IsFrontProcess()
{
    ProcessSerialNumber frontProcess;
    if (!GetFrontProcess(&frontProcess))
    {
        ProcessSerialNumber currentProcess;
        if (!GetCurrentProcess(&currentProcess))
        {
            return (frontProcess.highLongOfPSN == currentProcess.highLongOfPSN)
            && (frontProcess.lowLongOfPSN == currentProcess.lowLongOfPSN);
        }
    }
    
    return false;
}

bool IsOIGWindow(void* winId)
{
    NSView* view = (NSView*)winId;
    NSWindow* window = [view window];

    if (window)
    {
        // We use some of the properties set in the function SetupIGOWindow in IGOMacHelpers.mm to detect whether
        // the event is coming from an OIG window
        CGFloat alpha = [window alphaValue];
        BOOL isOpaque = [window isOpaque];
        BOOL ignoreMouseEvents = [window ignoresMouseEvents];
        
        if (!isOpaque && ignoreMouseEvents && alpha == 0.0f)
            return true;
    }
    
    return false;
}

bool checkSharedMemorySetup()
{
    return AllocateSharedMemory(false);
}

}
}
