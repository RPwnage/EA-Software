//
//  MacSystemPreferences.h
//  InjectedCode
//
//  Created by Frederic Meraud on 7/10/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef InjectedCode_MacSystemPreferences_h
#define InjectedCode_MacSystemPreferences_h

#include "IGO.h"

#if defined(ORIGIN_MAC)

class SystemPreferencesImpl;


class SystemPreferences
{
    public:
        enum HKID
        {
            HKID_PREVIOUS_INPUT_SOURCE = 60,
            HKID_NEXT_INPUT_SOURCE = 61
        };

    public:
        SystemPreferences();
        ~SystemPreferences();
    
        void Refresh();
        void GetHotKeys(SystemPreferences::HKID id, uint16_t* keyCode, uint64_t* modifiers);
    
    private:
        SystemPreferencesImpl* _impl;
};

#endif

#endif
