//
//  MacSystemPreferences.cpp
//  InjectedCode
//
//  Created by Frederic Meraud on 7/10/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "MacSystemPreferences.h"

#if defined(ORIGIN_MAC)

#include "MacWindows.h"

#include <Carbon/Carbon.h>

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>


struct HKDefinition
{
    SInt32          id;
    CGKeyCode       keyCode;
    CGEventFlags    modifiers;
};


static HKDefinition DefaultHotKeyDefinitions[] = 
{
    { SystemPreferences::HKID_PREVIOUS_INPUT_SOURCE, kVK_Space, kCGEventFlagMaskCommand },
    { SystemPreferences::HKID_NEXT_INPUT_SOURCE, kVK_Space, kCGEventFlagMaskCommand | kCGEventFlagMaskAlternate },
};

static const CFStringRef HKRootKeyword = CFSTR("AppleSymbolicHotKeys");
static const CFStringRef HKEnabledKeyword = CFSTR("enabled");
static const CFStringRef HKValueKeyword = CFSTR("value");
static const CFStringRef HKParametersKeyword = CFSTR("parameters");

enum HKDescriptionElementIndex
{
    HKDescriptionElementIndex_ASCII_VALUE = 0,
    HKDescriptionElementIndex_KEY_CODE,
    HKDescriptionElementIndex_MODIFIERS,
    
    HKDescriptionElementIndex_COUNT
};

////////////////////////////////////////////////////////////////////////////////////////////////////////


class SystemPreferencesImpl
{
    public:
        SystemPreferencesImpl()
            : _definitions(NULL), _fileId(-1), _queueId(-1), _definitionCnt(0)
        {
            _definitionCnt = sizeof(DefaultHotKeyDefinitions) / sizeof(HKDefinition);
            _definitions = new HKDefinition[_definitionCnt];

            _queueId = kqueue();
            if (_queueId != -1)
            {
                char homeDirectory[MAX_PATH];
                Origin::Mac::GetHomeDirectory(homeDirectory, sizeof(homeDirectory));
                snprintf(_symbolicHotKeysFileName, sizeof(_symbolicHotKeysFileName), "%s/Library/Preferences/com.apple.symbolichotkeys.plist", homeDirectory); 
            }
            
            else
            {
                IGO_TRACEF("Unable to create kqueue/watch system preferences (%d)", errno);
            }
            
            
            RefreshPreferences();
        }
        
        ~SystemPreferencesImpl()
        {
            if (_fileId != -1)
                close(_fileId);
            
            if (_queueId != -1)
                close(_queueId);
            
            delete[] _definitions;
        }
        
        void Refresh()
        {
            if (_fileId == -1)
            {
                _fileId = open(_symbolicHotKeysFileName, O_EVTONLY);
                if (_fileId == -1)
                {
                    IGO_TRACEF("Unable to open system preferences file '%s'/watch system preferences (%d)", _symbolicHotKeysFileName, errno);
                    return;
                }
            }
            
            struct kevent event;
            struct kevent notifyEvent;
            
            struct timespec timeout;
            timeout.tv_sec = 0;
            timeout.tv_nsec = 0;
            
            EV_SET(&notifyEvent, _fileId, EVFILT_VNODE, EV_ADD | EV_ONESHOT, NOTE_WRITE | NOTE_EXTEND | NOTE_ATTRIB | NOTE_DELETE | NOTE_LINK | NOTE_RENAME | NOTE_REVOKE, 0, 0);
            int eventCnt = kevent(_queueId, &notifyEvent, 1, &event, 1, &timeout);
            if (eventCnt == -1)
            {
                IGO_TRACEF("Error while pooling system preferences updates (%d)", errno);
            }
            
            else
            if (eventCnt > 0)
            {
                IGO_TRACE("Refreshing system preferences...");
                RefreshPreferences();
                
                if (event.fflags & NOTE_DELETE)
                {
                    close(_fileId);
                    _fileId = -1;
                }
            }
        }
        
        void GetHotKeys(SystemPreferences::HKID id, CGKeyCode* keyCode, CGEventFlags* modifiers)
        {
            for (int idx = 0; idx < _definitionCnt; ++idx)
            {
                if (_definitions[idx].id == id)
                {
                    if (keyCode)
                        *keyCode = _definitions[idx].keyCode;
                    
                    if (modifiers)
                        *modifiers = _definitions[idx].modifiers;
                    
                    return;
                }
            }
        }
            
    private:
        void SetDefaults()
        {
            for (int idx = 0; idx < _definitionCnt; ++idx)
            {
                _definitions[idx] = DefaultHotKeyDefinitions[idx];
            }
        }
    
        void RefreshPreferences()
        {
            SetDefaults();
            
            CFURLRef fileName = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (const UInt8*)_symbolicHotKeysFileName, strlen(_symbolicHotKeysFileName), false);
            if (fileName)
            {
                CFReadStreamRef stream = CFReadStreamCreateWithFile(kCFAllocatorDefault, fileName);
                if (stream)
                {
                    if (CFReadStreamOpen(stream))
                    {
                        CFPropertyListRef propertyList = CFPropertyListCreateWithStream(kCFAllocatorDefault, stream, 0, kCFPropertyListImmutable, NULL, NULL);
                        if (propertyList)
                        {
                            if (CFGetTypeID(propertyList) == CFDictionaryGetTypeID())
                            {
                                CFDictionaryRef rootDictionary = (CFDictionaryRef)CFDictionaryGetValue((CFDictionaryRef)propertyList, HKRootKeyword);
                                if (rootDictionary && CFGetTypeID(rootDictionary) == CFDictionaryGetTypeID())
                                {
                                    CFIndex hotKeyCount = CFDictionaryGetCount(rootDictionary);
                                    CFTypeRef* hotKeyIDList = new CFTypeRef[hotKeyCount];
                                    CFTypeRef* hotKeyInfoList = new CFTypeRef[hotKeyCount];
                                    CFDictionaryGetKeysAndValues(rootDictionary, (const void**)hotKeyIDList, (const void**)hotKeyInfoList);
                                    
                                    for (CFIndex hkIdx = 0; hkIdx < hotKeyCount; ++hkIdx)
                                    {
                                        if (CFGetTypeID(hotKeyIDList[hkIdx]) == CFStringGetTypeID()
                                            && CFGetTypeID(hotKeyInfoList[hkIdx]) == CFDictionaryGetTypeID())
                                        {
                                            CFDictionaryRef hotKeyInfo = (CFDictionaryRef)hotKeyInfoList[hkIdx];
                                            
                                            bool enabled = false;
                                            CFBooleanRef enabledTmp;
                                            if (CFDictionaryGetValueIfPresent(hotKeyInfo, HKEnabledKeyword, (const void**)&enabledTmp))
                                                enabled = CFBooleanGetValue(enabledTmp);

                                            if (enabled)
                                            {
                                                SInt32 keyID = CFStringGetIntValue((CFStringRef)hotKeyIDList[hkIdx]);
                                                for (int defIdx = 0; defIdx < _definitionCnt; ++defIdx)
                                                {
                                                    if (_definitions[defIdx].id == keyID)
                                                        ExtractKeyCodeAndModifiers(hotKeyInfo, &_definitions[defIdx]);
                                                }
                                            }
                                        }
                                    }
                                                        
                                    delete []hotKeyIDList;
                                    delete []hotKeyInfoList;
                                }
                            
                            }
                            
                            CFRelease(propertyList);
                        }
                    
                        CFReadStreamClose(stream);
                    }
                    
                    CFRelease(stream);
                }
                
                CFRelease(fileName);
            }

        }            
    
        void ExtractKeyCodeAndModifiers(CFDictionaryRef hotKeyInfo, HKDefinition* definition)
        {
            const void* valueDictionary = CFDictionaryGetValue(hotKeyInfo, HKValueKeyword);
            if (valueDictionary && CFGetTypeID(valueDictionary) == CFDictionaryGetTypeID())
            {
                const void* parametersArray = CFDictionaryGetValue((CFDictionaryRef)valueDictionary, HKParametersKeyword);
                if (parametersArray && CFGetTypeID(parametersArray) == CFArrayGetTypeID())
                {
                    const CFArrayRef parameters = (const CFArrayRef)parametersArray;
                    
                    if (CFArrayGetCount(parameters) == HKDescriptionElementIndex_COUNT)
                    {
                        CGKeyCode keyCode;
                        CFNumberRef keyCodeWrapper = (CFNumberRef)CFArrayGetValueAtIndex(parameters, HKDescriptionElementIndex_KEY_CODE);                      
                        if (CFNumberGetValue(keyCodeWrapper, kCFNumberSInt16Type, &keyCode))
                        {
                            CGEventFlags modifiers;
                            CFNumberRef modifiersWrapper = (CFNumberRef)CFArrayGetValueAtIndex(parameters, HKDescriptionElementIndex_MODIFIERS);
                            if (CFNumberGetValue(modifiersWrapper, kCFNumberSInt64Type, &modifiers))
                            {
                                definition->keyCode = keyCode;
                                definition->modifiers = modifiers;
                            }
                        }

                    }
                }
            }
        }
    
    
    
    
        char _symbolicHotKeysFileName[MAX_PATH];
    
        HKDefinition* _definitions;
    
        int _fileId;
        int _queueId;
        int _definitionCnt;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////

SystemPreferences::SystemPreferences()
: _impl(new SystemPreferencesImpl())
{
}

SystemPreferences::~SystemPreferences()
{
    delete _impl;
}

void SystemPreferences::Refresh()
{
    _impl->Refresh();
}

void SystemPreferences::GetHotKeys(SystemPreferences::HKID id, uint16_t* keyCode, uint64_t* modifiers)
{
    _impl->GetHotKeys(id, keyCode, modifiers);
}

#endif