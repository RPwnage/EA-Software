#include "Stroke.h"
#include <SonarInput/Input.h>
#include <assert.h>
#include <SonarCommon/Common.h>

#include <cctype>


namespace sonar
{
// trim from both ends
static inline String trim(const String &s) 
{
	size_t offset = 0;
	size_t len = s.size();

	while (offset < len && isspace(s[offset]))
		offset ++;

	size_t start = offset;

	size_t end = len;

	if (len > 0)
	{
		offset = len - 1;

		while (offset > start && isspace(s[offset]))
			offset --;

		end = offset;
	}
	return s.substr(start, (end - start) + 1);
}

void Stroke::baseInit(void)
{
	if (m_nameToKey.empty())
	{
		init();
	}

	m_iter = 0;
	m_block = false;
}


Stroke::Stroke(void)
{
	baseInit();

    m_use_virtual_keys = false;
}

Stroke::Stroke(CString &stroke, bool block)
{
	baseInit();

    m_use_virtual_keys = false;

	m_block = block;
	
	if (m_nameToKey.empty())
	{
		init();
	}

	TokenList tokens = common::tokenize(stroke, '+');

	for (TokenList::iterator iter = tokens.begin(); iter != tokens.end(); iter ++)
	{
		String keyName = trim(*iter);

		NameToKeyMap::iterator nameIt = m_nameToKey.find(keyName);

		if (nameIt == m_nameToKey.end())
		{
			m_keys.clear();
			return;
		}

		m_keys.push_back(nameIt->second);
	}
}

Stroke::Stroke(int vk_strokes[], int size, bool block)
{
    baseInit();

    m_use_virtual_keys = true;
	m_block = block;

    for (int i = 0; i < size; i++)
    {
        m_keys.push_back(vk_strokes[i]);
    }
}

void Stroke::reset()
{
	m_iter = 0;
}

bool Stroke::isFinal()
{
	if (m_keys.empty())
	{
		return false;
	}

	return m_iter == (int) m_keys.size();
}

int process_virtualkey(int virtual_key)
{
    switch (virtual_key)
    {
    case VK_LSHIFT:
    case VK_RSHIFT:
        return VK_SHIFT;
    case VK_LCONTROL:
    case VK_RCONTROL:
        return VK_CONTROL;
    case VK_LMENU:
    case VK_RMENU:
        return VK_MENU;
    case VK_LWIN:
    case VK_RWIN:
        return VK_RWIN;
    }

    return virtual_key;
}

bool Stroke::pressKey(int key, int virtual_key)
{
	if (isFinal())
	{
		return false;
	}

    if (m_use_virtual_keys)
        key = process_virtualkey(virtual_key);

	if (m_keys[m_iter] != key)
	{
		return false;
	}

	m_iter ++;
	return true;
}

bool Stroke::releaseKey(int key, int virtual_key)
{
    if (m_use_virtual_keys)
        key = process_virtualkey(virtual_key);

	for (int index = 0; index < m_iter; index ++)
	{
		if (m_keys[index] == key)
		{
			m_iter = index;
			return true;
		}
	}

	return false;
}

void Stroke::recordKey(int key)
{
	m_keys.push_back(key);
}

const Stroke::KeyList &Stroke::getKeys() const
{
	return m_keys;
}

bool Stroke::shouldBlock()
{
	return m_block;
}

void Stroke::clear()
{
	m_keys.clear();	
}

String Stroke::toString(void)
{
	StringStream ss;

	for (KeyList::iterator iter = m_keys.begin(); iter != m_keys.end(); iter ++)
	{
		if (iter != m_keys.begin())
		{
			ss << " + ";
		}

		KeytoNameMap::iterator nameIter = m_keyToName.find(*iter);
		
		if (nameIter == m_keyToName.end())
		{
				ss << "(" << *iter << ")";
				continue;
		}

		ss << nameIter->second;
	}

	return ss.str();
}

Stroke::NameToKeyMap Stroke::m_nameToKey;
Stroke::KeytoNameMap Stroke::m_keyToName;

void Stroke::init()
{
	// iterrate all keys to string and save lookup

	for (int key = 1; key < KEY_MS_START; key ++)

	{
		LONG param = key << 16;

		wchar_t wstrKey[256 + 1];
		if (!::GetKeyNameTextW(param, wstrKey, 256))
		{
			continue;
		}

		String strKey = common::wideToUtf8(wstrKey);
		
		m_nameToKey[strKey] = key;
	}

	m_nameToKey["mouse1"] = KEY_LBUTTON;
	m_nameToKey["mouse2"] = KEY_MBUTTON;
    m_nameToKey["mouse3"] = KEY_RBUTTON;
	m_nameToKey["mouse4"] = KEY_XBUTTON1;
	m_nameToKey["mouse5"] = KEY_XBUTTON2;

    // fix '\' (backslash)
    m_nameToKey["\\"] = 43;
    // fix 'Menu'
    m_nameToKey["Menu"] = 93;

	for (NameToKeyMap::iterator iter = m_nameToKey.begin(); iter != m_nameToKey.end(); iter ++)
	{
		m_keyToName[iter->second] = iter->first;
	}

}


CString Stroke::getKeyName(int key)
{
	KeytoNameMap::iterator iter = m_keyToName.find(key);

	if (iter == m_keyToName.end())
	{
		return "";
	}

	return iter->second;
}

}
