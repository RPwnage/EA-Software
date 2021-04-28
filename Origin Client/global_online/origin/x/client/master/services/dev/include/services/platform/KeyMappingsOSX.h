//
//  KeyMappingsOSX.h
//  engine 
//
//  Created by Frederic Meraud on 6/25/12.
//  Copyright (c) 2012 EA. All rights reserved.
//

#ifndef Engine_KeyMappingsOSX_h
#define Engine_KeyMappingsOSX_h

#include <Carbon/Carbon.h>

#include "services/plugin/PluginAPI.h"

/*
 * Virtual Keys, Standard Set
 */
//#define VK_LBUTTON        
//#define VK_RBUTTON        
//#define VK_CANCEL         
//#define VK_MBUTTON            /* NOT contiguous with L & RBUTTON */

//#define VK_XBUTTON1           /* NOT contiguous with L & RBUTTON */
//#define VK_XBUTTON2           /* NOT contiguous with L & RBUTTON */

/*
 * 0x07 : unassigned
 */

#define VK_BACK           kVK_Delete
#define VK_TAB            kVK_Tab

/*
 * 0x0A - 0x0B : reserved
 */

#define VK_CLEAR          kVK_ANSI_KeypadClear
#define VK_RETURN         kVK_Return

#define VK_SHIFT          kVK_Shift
#define VK_CONTROL        kVK_Control
#define VK_MENU           kVK_Option
#define VK_PAUSE          UNUSED_VKEY_ENTRY
#define VK_CAPITAL        kVK_CapsLock

//#define VK_KANA         
//#define VK_HANGEUL      
//#define VK_HANGUL       
//#define VK_JUNJA        
//#define VK_FINAL        
//#define VK_HANJA        
//#define VK_KANJI        

#define VK_ESCAPE         kVK_Escape

//#define VK_CONVERT      
//#define VK_NONCONVERT   
//#define VK_ACCEPT       
//#define VK_MODECHANGE   

#define VK_SPACE          kVK_Space
#define VK_PRIOR          kVK_PageUp
#define VK_NEXT           kVK_PageDown
#define VK_END            kVK_End
#define VK_HOME           kVK_Home
#define VK_LEFT           kVK_LeftArrow
#define VK_UP             kVK_UpArrow
#define VK_RIGHT          kVK_RightArrow
#define VK_DOWN           kVK_DownArrow
//#define VK_SELECT         
//#define VK_PRINT          
//#define VK_EXECUTE        
//#define VK_SNAPSHOT       
//#define VK_INSERT         
#define VK_DELETE         kVK_ForwardDelete
//#define VK_HELP           

/*
 * VK_0 - VK_9 are the same as ASCII '0' - '9' (0x30 - 0x39)
 * 0x40 : unassigned
 * VK_A - VK_Z are the same as ASCII 'A' - 'Z' (0x41 - 0x5A)
 */
#define VK_S              kVK_ANSI_S    // for ANSI US ONLY

#define VK_LWIN           kVK_Command
#define VK_RWIN           0x36 // NOT DEFINED IN .H!
//#define VK_APPS           

/*
 * 0x5E : reserved
 */

//#define VK_SLEEP          

#define VK_NUMPAD0        kVK_ANSI_Keypad0
#define VK_NUMPAD1        kVK_ANSI_Keypad1
#define VK_NUMPAD2        kVK_ANSI_Keypad2
#define VK_NUMPAD3        kVK_ANSI_Keypad3
#define VK_NUMPAD4        kVK_ANSI_Keypad4
#define VK_NUMPAD5        kVK_ANSI_Keypad5
#define VK_NUMPAD6        kVK_ANSI_Keypad6
#define VK_NUMPAD7        kVK_ANSI_Keypad7
#define VK_NUMPAD8        kVK_ANSI_Keypad8
#define VK_NUMPAD9        kVK_ANSI_Keypad9
#define VK_MULTIPLY       kVK_ANSI_KeypadMultiply
#define VK_ADD            kVK_ANSI_KeypadPlus
//#define VK_SEPARATOR      
#define VK_SUBTRACT       kVK_ANSI_KeypadMinus
#define VK_DECIMAL        kVK_ANSI_KeypadDecimal
#define VK_DIVIDE         kVK_ANSI_KeypadDivide
#define VK_F1             kVK_F1
#define VK_F2             kVK_F2
#define VK_F3             kVK_F3
#define VK_F4             kVK_F4
#define VK_F5             kVK_F5
#define VK_F6             kVK_F6
#define VK_F7             kVK_F7
#define VK_F8             kVK_F8
#define VK_F9             kVK_F9
#define VK_F10            kVK_F10
#define VK_F11            kVK_F11
#define VK_F12            kVK_F12
#define VK_F13            kVK_F13
#define VK_F14            kVK_F14
#define VK_F15            kVK_F15
#define VK_F16            kVK_F16
#define VK_F17            kVK_F17
#define VK_F18            kVK_F18
#define VK_F19            kVK_F19
#define VK_F20            kVK_F20
//#define VK_F21            
//#define VK_F22            
//#define VK_F23            
//#define VK_F24            

/*
 * 0x88 - 0x8F : unassigned
 */

//#define VK_NUMLOCK      
//#define VK_SCROLL       

/*
 * NEC PC-9800 kbd definitions
 */
#define VK_OEM_NEC_EQUAL  kVK_ANSI_KeypadEquals   // '=' key on numpad

/*
 * Fujitsu/OASYS kbd definitions
 */
//#define VK_OEM_FJ_JISHO      // 'Dictionary' key
//#define VK_OEM_FJ_MASSHOU    // 'Unregister word' key
//#define VK_OEM_FJ_TOUROKU    // 'Register word' key
//#define VK_OEM_FJ_LOYA       // 'Left OYAYUBI' key
//#define VK_OEM_FJ_ROYA       // 'Right OYAYUBI' key

/*
 * 0x97 - 0x9F : unassigned
 */

/*
 * VK_L* & VK_R* - left and right Alt, Ctrl and Shift virtual keys.
 * Used only as parameters to GetAsyncKeyState() and GetKeyState().
 * No other API or message will distinguish left and right keys in this way.
 */
#define VK_LSHIFT         kVK_Shift
#define VK_RSHIFT         kVK_RightShift
#define VK_LCONTROL       kVK_Control
#define VK_RCONTROL       kVK_RightControl
#define VK_LMENU          kVK_Option
#define VK_RMENU          kVK_RightOption

//#if(_WIN32_WINNT >= 0x0500)
//#define VK_BROWSER_BACK        
//#define VK_BROWSER_FORWARD     
//#define VK_BROWSER_REFRESH     
//#define VK_BROWSER_STOP        
//#define VK_BROWSER_SEARCH      
//#define VK_BROWSER_FAVORITES   
//#define VK_BROWSER_HOME        

#define VK_VOLUME_MUTE         kVK_Mute
#define VK_VOLUME_DOWN         kVK_VolumeDown
#define VK_VOLUME_UP           kVK_VolumeUp
//#define VK_MEDIA_NEXT_TRACK    
//#define VK_MEDIA_PREV_TRACK    
//#define VK_MEDIA_STOP          
//#define VK_MEDIA_PLAY_PAUSE    
//#define VK_LAUNCH_MAIL         
//#define VK_LAUNCH_MEDIA_SELECT 
//#define VK_LAUNCH_APP1         
//#define VK_LAUNCH_APP2         

//#endif /* _WIN32_WINNT >= 0x0500 */

/*
 * 0xB8 - 0xB9 : reserved
 */

#define VK_OEM_1          kVK_ANSI_Semicolon   // ';:' for US
//#define VK_OEM_PLUS          // '+' any country
//#define VK_OEM_COMMA         // ',' any country
//#define VK_OEM_MINUS         // '-' any country
//#define VK_OEM_PERIOD        // '.' any country
#define VK_OEM_2          kVK_ANSI_Slash   // '/?' for US
#define VK_OEM_3          kVK_ANSI_Grave   // '`~' for US

/*
 * 0xC1 - 0xD7 : reserved
 */

/*
 * 0xD8 - 0xDA : unassigned
 */

#define VK_OEM_4          kVK_ANSI_LeftBracket   //  '[{' for US
#define VK_OEM_5          kVK_ANSI_Backslash     //  '\|' for US
#define VK_OEM_6          kVK_ANSI_RightBracket  //  ']}' for US
#define VK_OEM_7          kVK_ANSI_Quote         //  ''"' for US
//#define VK_OEM_8        

/*
 * 0xE0 : reserved
 */

/*
 * Various extended or enhanced keyboards
 */
//#define VK_OEM_AX           //  'AX' key on Japanese AX kbd
#define VK_OEM_102        kVK_ANSI_Backslash  //  "<>" or "\|" on RT 102-key kbd.
#define VK_ICO_HELP       kVK_Help  //  Help key on ICO
//#define VK_ICO_00           //  00 key on ICO

//#if(WINVER >= 0x0400)
//#define VK_PROCESSKEY     
//#endif /* WINVER >= 0x0400 */

#define VK_ICO_CLEAR      kVK_ANSI_KeypadClear


//#if(_WIN32_WINNT >= 0x0500)
//#define VK_PACKET         
//#endif /* _WIN32_WINNT >= 0x0500 */

/*
 * 0xE8 : unassigned
 */

/*
 * Nokia/Ericsson definitions
 */
//#define VK_OEM_RESET      
//#define VK_OEM_JUMP       
//#define VK_OEM_PA1        
//#define VK_OEM_PA2        
//#define VK_OEM_PA3        
//#define VK_OEM_WSCTRL     
//#define VK_OEM_CUSEL      
//#define VK_OEM_ATTN       
//#define VK_OEM_FINISH     
//#define VK_OEM_COPY       
//#define VK_OEM_AUTO       
//#define VK_OEM_ENLW       
//#define VK_OEM_BACKTAB    

//#define VK_ATTN           
//#define VK_CRSEL          
//#define VK_EXSEL          
//#define VK_EREOF          
//#define VK_PLAY           
//#define VK_ZOOM           
//#define VK_NONAME         
//#define VK_PA1            
//#define VK_OEM_CLEAR      

/*
 * 0xFF : reserved
 */

#endif // Engine_KeyMappingsOSX_h
