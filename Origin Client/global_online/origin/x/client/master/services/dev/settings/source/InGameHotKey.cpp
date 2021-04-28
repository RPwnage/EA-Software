#include "InGameHotKey.h"

#include <QObject>
#include "services/log/LogService.h"

#if defined(Q_OS_WIN)
#include "Windows.h"
#elif defined(ORIGIN_MAC)
#include "services/platform/KeyMappingsOSX.h"
#endif


namespace Origin
{
    namespace Services
    {
	    InGameHotKey::InGameHotKey(int32_t qtModifier, int32_t qtKey, int32_t winVirtualKey)
		    : mQtHotKey(qtModifier | qtKey)
		    , mWinHotKey(winVirtualKey)
            , mStatus(NotValid_Ignore)
            , mFirstKey(-1)
            , mLastKey(-1)
	    {
			setupHotkeyDisplayNameMap();
	        mStatus = setupHotkey(qtModifier, winVirtualKey, qtKey);
	    }

	    InGameHotKey::InGameHotKey()
		    : mQtHotKey(0)
		    , mWinHotKey(0)
	    {
	    }

	    InGameHotKey::InGameHotKey(const InGameHotKey& from)
		    : mQtHotKey(from.mQtHotKey)
		    , mWinHotKey(from.mWinHotKey)
            , mFirstKey(from.mFirstKey)
            , mLastKey(from.mLastKey)
	    {
	    }

	    QString InGameHotKey::asNativeString() const
	    {
		    // Note: We break it down into three separate parts to allow us to present a pre-modifier key
		    // version of the hot-key to the user; e.g. Shift+1 => "Shift+1" instead of "Shift+!"
		    int32_t modifierKey = QtModifierKey();
		    bool isKeyPad = (modifierKey & Qt::KeypadModifier) == Qt::KeypadModifier;
		    modifierKey = (modifierKey & ~Qt::KeypadModifier);
		    int32_t unmodifiedKey = QtUnmodifiedKey();

		    // Convert modifier key to string manually instead of going through Qt for the time being
		    // to avoid Qt's "occasional" localization - will be all English for now. Note: this code
		    // will also support multiple modifier keys although the IGO implementation only supports
		    // one (for now).
            QString modifier;
            if (modifierKey & Qt::ShiftModifier)
            {
                modifier += QObject::tr("ebisu_client_shortcut_shift") + "+";
            }
            if (modifierKey & Qt::ControlModifier)
            {
                modifier += QObject::tr("ebisu_client_shortcut_ctrl") + "+";
            }
            if (modifierKey & Qt::AltModifier)
            {
#if defined(ORIGIN_MAC)
                modifier += QObject::tr("ebisu_client_shortcut_option") + "+";
#else
                modifier += QObject::tr("ebisu_client_shortcut_alt") + "+";
#endif
            }
#if defined(ORIGIN_MAC)
            if (modifierKey & Qt::MetaModifier)
            {
                modifier += QObject::tr("ebisu_client_shortcut_command") + "+";
            }

#endif
        
		    QString keypad;
			if (isKeyPad && unmodifiedKey != Qt::Key_NumLock && unmodifiedKey != Qt::Key_Enter)
		    {
			    keypad = QObject::tr("ebisu_client_hot_key_key_pad");
		    }

		    QString unmodified;
		    if (unmodifiedKey == Qt::Key_Cancel)
		    {
			    // For some reason, Qt reads "Scroll-Lock" as the "Cancel" key when going through the
			    // Japanese IME but Qt 4.7.1 doesn't have a string version of the Cancel key so we have
			    // to synthesize one.
			    unmodified = QObject::tr("ebisu_client_hot_key_scroll_lock");
		    }
		    else
		    {
			    if( mHotKeyDisplayName.find(unmodifiedKey) != mHotKeyDisplayName.constEnd())
			    {
				    unmodified = QObject::tr(mHotKeyDisplayName.value(unmodifiedKey).toUtf8());
			    }
			    else
			    {
				    QKeySequence keqSeq(unmodifiedKey);
				    unmodified = QObject::tr(keqSeq.toString(QKeySequence::NativeText).toUtf8());
			    }

		    }

		    return modifier + keypad + unmodified;
	    }

		QString InGameHotKey::keyDisplayName(int32_t key)
		{
			QString keyName = "";
			setupHotkeyDisplayNameMap();
			if( mHotKeyDisplayName.find(key) != mHotKeyDisplayName.constEnd())
			{
				keyName = QObject::tr(mHotKeyDisplayName.value(key).toUtf8());
			}
			else
			{
			    QKeySequence keqSeq(key);
			    keyName = QObject::tr(keqSeq.toString(QKeySequence::NativeText).toUtf8());
			}

			return keyName;
		}
		
        bool InGameHotKey::isAllowedModifier(int32_t qtKey)
        {
            bool allowed = (qtKey == Qt::Key_Control || qtKey == Qt::Key_Shift);
#if defined(ORIGIN_MAC)
            allowed = allowed || (qtKey == Qt::Key_Meta);
#endif
            return allowed;
        }

        InGameHotKey::HotkeyStatus InGameHotKey::setupHotkey(int32_t qtModifier, int32_t winVirtualKey, int32_t qtKey)
        {
            // ignore multi-modifier keys and the windows key for the time being -- REVISIT
            bool isModifier = (qtKey == Qt::Key_Control || qtKey == Qt::Key_Shift || qtKey == Qt::Key_Alt);
            bool hasNoModifier = false;
            bool hasOneModifier = false;

    #if defined(ORIGIN_MAC)
            if (qtKey == Qt::Key_Meta)
            {
                isModifier = true;

                // Qt intercepts Ctrl+Tab to use with its QMdiArea object, so we use a tap to detect that combo and resend a ctrl key
                // press down event so that we have the opportunity to check the tab key state.
                bool hasTab = CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState, kVK_Tab);
                if (hasTab)
                {
                    isModifier = false;
                    winVirtualKey  = kVK_Tab;
                    qtKey  = Qt::Key_Tab;
                }
            }
            
            // for OSX, the Command key is mapped to Qt::Key_Control but the modifier is set to Qt::MetaModifier
            // which is making things a little too complicated.  So we will ignore the standalone Command key as a hotkey
            if (qtKey == Qt::Key_Control)
            {
                return NotValid_Ignore;
            }
            
    #endif

            if ((isModifier && !isAllowedModifier(qtKey)) || qtKey == 0 || qtKey == Qt::Key_unknown)
            {
                return NotValid_Ignore;
            }

            if (qtKey == Qt::Key_Meta)
            {
                return NotValid_Ignore;
            }

    #if defined(ORIGIN_MAC)
            // NOTE: Qt doesn't seem to handle the Keypad Enter key correctly -> it sets the modifier to Qt::GroupSwitchModifier (which from the doc
            // should only be used for X11) and the virtual key to 0. Not sure if this is the only case that would cause this combo, so I'll wait
            // until somebody decides it's important to support it for hot keys (knowing we already limit the user, don't think it would really matter)
            // We could also patch Qt to handle this.

            // Can't rely on the Qt::KeypadModifier for Mac - so directly check the virtual code
            if (winVirtualKey >= kVK_ANSI_KeypadDecimal && winVirtualKey <= kVK_ANSI_Keypad9)
                qtModifier |= Qt::KeypadModifier;

            int modifiers = (qtModifier & (Qt::ControlModifier | Qt::ShiftModifier | Qt::AltModifier | Qt::MetaModifier));
            hasOneModifier = (modifiers == Qt::ControlModifier || modifiers == Qt::ShiftModifier || modifiers == Qt::AltModifier || modifiers == Qt::MetaModifier);
    #else
            int modifiers = (qtModifier & (Qt::ControlModifier | Qt::ShiftModifier | Qt::AltModifier));
            hasOneModifier = (modifiers == Qt::ControlModifier || modifiers == Qt::ShiftModifier || modifiers == Qt::AltModifier);
    #endif

            // ignore keypad modifier when testing for modifier keys
            hasNoModifier = (qtModifier & ~Qt::KeypadModifier) == Qt::NoModifier;

            if (hasNoModifier || hasOneModifier)
            {
                if (qtKey >= Qt::Key_Space && qtKey <= Qt::Key_AsciiTilde)
                {
#if defined(Q_OS_WIN)
                    // This code remaps the unmodified virtual key to a Qt key so that we end up displaying
                    // Shift+1 as "Shift+1", not as "Shift+!".
                    QChar mappedCode = static_cast<QChar>(MapVirtualKeyW(winVirtualKey, MAPVK_VK_TO_CHAR));
                    qtKey = mappedCode.toUpper().unicode();

#elif defined(ORIGIN_MAC)
                    // Lookup the current keyboard input source
                    TISInputSourceRef inputSource = TISCopyCurrentKeyboardInputSource();
                    if (inputSource)
                    {
                        // Get its layout for display
                        CFDataRef data = static_cast<CFDataRef>(TISGetInputSourceProperty(inputSource, kTISPropertyUnicodeKeyLayoutData));
                        if (data)
                        {
                            // Get the unicode representation when no modifier is applied to the original virtual key
                            UCKeyboardLayout* kbdLayout = reinterpret_cast<UCKeyboardLayout *>(const_cast<UInt8 *>(CFDataGetBytePtr(data)));

                            UInt32 unused = 0;
                            UInt32 modifiers = 0;

                            // The current implementation displays a single character
                            UniCharCount stringLen = 1;
                            UniChar uPressKey[stringLen];

                            OSStatus err = UCKeyTranslate(kbdLayout, static_cast<UInt16>(winVirtualKey), kUCKeyActionDisplay, modifiers, LMGetKbdType(),
                                kUCKeyTranslateNoDeadKeysBit, &unused, stringLen, &stringLen, uPressKey);

                            if (err == noErr)
                            {
                                // We are already handling "control" entries somewhere else (tab, escape, enter, ...)
                                if (uPressKey[0] > 0x20 && uPressKey[0] != 0x7f)
                                    qtKey = uPressKey[0];
                            }
                        }

                        CFRelease(inputSource);
                    }
    #endif
                }
            }
            else
            {
                // ignore keys we don't handle
                return NotValid_Ignore;
            }

            if (QtModifierKey() & Qt::ShiftModifier)
            {
                mFirstKey = VK_SHIFT;
            }
            else if (QtModifierKey() & Qt::ControlModifier)
            {
                mFirstKey = VK_CONTROL;
            }
            else if (QtModifierKey() & Qt::AltModifier)
            {
                mFirstKey = VK_MENU; // (OPTION key for MAC)
            }
            else if (QtModifierKey() & Qt::MetaModifier)
            {
    #if defined(ORIGIN_MAC)        
                mFirstKey = VK_LWIN; // COMMAND key
    #else
                mFirstKey = VK_RWIN;
    #endif
            }
            else
            {
                mFirstKey = WinVirtualKey();
            }

            if (mFirstKey != WinVirtualKey())
                mLastKey = WinVirtualKey();
            else
                mLastKey = mFirstKey;

            if (VK_ESCAPE == mFirstKey && VK_ESCAPE == mLastKey)
            {
                return NotValid_Revert;
            }

    #if defined(ORIGIN_PC)
            // don't allow alt+enter as a hotkey on the PC - might conflict with alt+enter that is typically used to toggle between fullscreen & windowed mode
            if (VK_RETURN == mFirstKey && VK_MENU == mLastKey || VK_RETURN == mLastKey && VK_MENU == mFirstKey)
            {
                return NotValid_Revert;
            }
    #endif

            // prevent double modifier keys, i.e. Ctrl+Ctrl or Shift+Shift
            if( (qtKey == Qt::Key_Control && qtModifier == Qt::ControlModifier) ||
                (qtKey == Qt::Key_Shift && qtModifier == Qt::ShiftModifier)
    #if defined(ORIGIN_MAC)
                || (qtKey == Qt::Key_Meta && qtModifier == Qt::MetaModifier)
    #endif
              )
            {
                qtModifier = 0;
            }

            mQtHotKey = qtModifier | qtKey;
            mWinHotKey = winVirtualKey;

            return Valid;
        }

		void InGameHotKey::setupHotkeyDisplayNameMap()
		{
			if(mHotKeyDisplayName.isEmpty())
		    {
			    mHotKeyDisplayName.insert(Qt::Key_Delete, "ebisu_client_hot_key_delete");
			    mHotKeyDisplayName.insert(Qt::Key_Insert, "ebisu_client_hot_key_insert");
			    mHotKeyDisplayName.insert(Qt::Key_Home, "ebisu_client_hot_key_home");
			    mHotKeyDisplayName.insert(Qt::Key_End, "ebisu_client_hot_key_end");
			    mHotKeyDisplayName.insert(Qt::Key_PageUp, "ebisu_client_hot_key_page_up");
			    mHotKeyDisplayName.insert(Qt::Key_PageDown, "ebisu_client_hot_key_page_down");
			    mHotKeyDisplayName.insert(Qt::Key_Return, "ebisu_client_hot_key_enter");
			    mHotKeyDisplayName.insert(Qt::Key_Enter, "ebisu_client_hot_key_enter");
			    mHotKeyDisplayName.insert(Qt::Key_Left, "ebisu_client_hot_key_left");
			    mHotKeyDisplayName.insert(Qt::Key_Right, "ebisu_client_hot_key_right");
			    mHotKeyDisplayName.insert(Qt::Key_Up, "ebisu_client_hot_key_up");
			    mHotKeyDisplayName.insert(Qt::Key_Down, "ebisu_client_hot_key_down");
			    mHotKeyDisplayName.insert(Qt::Key_ScrollLock, "ebisu_client_hot_key_scroll_lock");
			    mHotKeyDisplayName.insert(Qt::Key_Pause, "ebisu_client_hot_key_pause");
			    mHotKeyDisplayName.insert(Qt::Key_CapsLock, "ebisu_client_hot_key_caps_lock");
			    mHotKeyDisplayName.insert(Qt::Key_Menu, "ebisu_client_hot_key_menu");
			    mHotKeyDisplayName.insert(Qt::Key_NumLock, "ebisu_client_hot_key_num_lock");
			    mHotKeyDisplayName.insert(Qt::Key_Space, "ebisu_client_hot_key_space");
			    mHotKeyDisplayName.insert(Qt::Key_Escape, "ebisu_client_hot_key_escape");
#ifdef ORIGIN_MAC
                mHotKeyDisplayName.insert(Qt::Key_Backspace, "ebisu_client_hot_key_delete");
#else
			    mHotKeyDisplayName.insert(Qt::Key_Backspace, "ebisu_client_hot_key_backspace");
#endif
                mHotKeyDisplayName.insert(Qt::Key_Tab, "ebisu_client_hot_key_tab");
                mHotKeyDisplayName.insert(Qt::Key_Backtab, "ebisu_client_hot_key_tab");

                // modifiers
                mHotKeyDisplayName.insert(Qt::Key_Control, "ebisu_client_shortcut_ctrl");
                mHotKeyDisplayName.insert(Qt::Key_Shift, "ebisu_client_shortcut_shift");
#if defined(ORIGIN_MAC)
                mHotKeyDisplayName.insert(Qt::Key_Meta, "ebisu_client_shortcut_command");
#endif
		    }
		}

        void InGameHotKey::getVirtualKeyArray(int vk_keys[], int &size)
        {
            int input_size = size;

            if (input_size == 0)
                return;

            vk_keys[0] = mFirstKey;
            size = 1;

            if (input_size == 1)
                return;

            if (mFirstKey != mLastKey)
            {
                vk_keys[1] = mLastKey;
                size = 2;
            }
        }
	}
}