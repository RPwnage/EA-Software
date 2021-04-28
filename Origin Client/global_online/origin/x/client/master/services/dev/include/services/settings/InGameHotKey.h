//  Copyright (c) 2011 Electronic Arts. All rights reserved.

#ifndef __INGAMEHOTKEY__
#define __INGAMEHOTKEY__

#include "EABase/eabase.h"
#include <qstring.h>
#include <qkeysequence.h>
#include <QMap>

#include "services/plugin/PluginAPI.h"
namespace Origin
{

    namespace Services {
    // TBD: The following class needs to be relocated somewhere more appropriate
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary>	InGameHotKey manages the definition of what constitutes an in-game hot key. </summary>
    /// <remarks>	Created by Bela Kiss. </remarks>
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    class ORIGIN_PLUGIN_API InGameHotKey
    {
    public:

	    InGameHotKey(int32_t qtModifier, int32_t qtKey, int32_t winVirtualKey);
	    InGameHotKey();
	    InGameHotKey(const InGameHotKey& from);

        enum HotkeyStatus
        {
            NotValid_Ignore,
            NotValid_Revert,
            Valid
        };

	    int32_t QtModifierKey() const { return mQtHotKey & Qt::KeyboardModifierMask; }
	    int32_t QtUnmodifiedKey() const { return mQtHotKey & ~Qt::KeyboardModifierMask; }
	    int32_t WinVirtualKey() const { return mWinHotKey; }
	    int32_t QtModifiedKey() const { return mQtHotKey; }

	    QString asPortableString() const
	    {
		    QKeySequence keqSeq(QtModifiedKey());
		    return keqSeq.toString(QKeySequence::PortableText);
	    }

	    QString asNativeString() const;
		QString keyDisplayName(int32_t);

        HotkeyStatus status() const {return mStatus;}
        int firstKey() const {return mFirstKey;}
        int lastKey() const {return mLastKey;}

	    // Conversion routines for the Settings Component
	    unsigned long long toULongLong() const
	    {
		    return (static_cast<unsigned long long>(mQtHotKey) << 32) | mWinHotKey;
	    }

	    static InGameHotKey fromULongLong(unsigned long long val)
	    {
		    return InGameHotKey((val >> 32) & Qt::KeyboardModifierMask, (val >> 32) & ~Qt::KeyboardModifierMask, val & 0xffffffff);
	    }

        void getVirtualKeyArray(int vk_keys[], int &size);

    private:
        HotkeyStatus setupHotkey(int32_t qtModifier, int32_t winVirtualKey, int32_t qtKey);
        void setupHotkeyDisplayNameMap();
        bool isAllowedModifier(int32_t qtKey);

	    uint32_t mQtHotKey;			// using Qt's definition of a key including the modifier
	    uint32_t mWinHotKey;		// using Windows' definition of an unmodified virtual key
        HotkeyStatus mStatus;
        int mFirstKey;
        int mLastKey;
		
	    QMap<int32_t, QString> mHotKeyDisplayName;
    };
    } // namespace Services

} // namespace Origin

#endif // __INGAMEHOTKEY__