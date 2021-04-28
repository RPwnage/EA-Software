#pragma once

#include <SonarCommon/Common.h>

namespace sonar
{

	class Stroke
	{

	public:
		Stroke(void);
		Stroke(CString &stroke, bool block);
        Stroke(int vk_strokes[], int size, bool block);

		typedef SonarVector<int> KeyList;

		void reset();
		bool isFinal();
		bool pressKey(int key, int virtual_key);
		bool releaseKey(int key, int virtual_key);
		void recordKey(int key);
		const KeyList &getKeys() const;
		bool shouldBlock();
		void clear();
		String toString(void);
		
		const static int KEY_KB_START = 0x00;
		const static int KEY_ESCAPE = 0x01;
		const static int KEY_MS_START = 0x100;
		const static int KEY_LBUTTON = KEY_MS_START + 0x00;
		const static int KEY_RBUTTON = KEY_MS_START + 0x01;
		const static int KEY_MBUTTON = KEY_MS_START + 0x02;
		const static int KEY_XBUTTON1 = KEY_MS_START + 0x03;
		const static int KEY_XBUTTON2 = KEY_MS_START + 0x04;
		const static int KEY_MAX = 0x10f;

		static CString getKeyName(int key);

	private:
		
		void baseInit(void);

		KeyList m_keys;
		bool m_block;
        bool m_use_virtual_keys;
		int m_iter;

		static void init();
		typedef SonarMap<CString, int> NameToKeyMap;
		typedef SonarMap<int, String> KeytoNameMap;
		static NameToKeyMap m_nameToKey;
		static KeytoNameMap m_keyToName;


	};

}