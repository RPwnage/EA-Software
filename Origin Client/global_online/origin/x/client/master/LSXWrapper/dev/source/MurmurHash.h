#ifndef _MURMUR_HASH_
#define _MURMUR_HASH_

#include <string>

namespace Hash
{
	typedef std::string TString;
	typedef unsigned long THash;

	unsigned int MurmurHash2 ( const void * key, size_t len, unsigned int seed );
	THash GetHash(const char * text);

	class String
	{
	public:
		String()
		{
			m_Hash = 0;
		}
		String(const char * str)
		{
			if(str != NULL)
			{
				m_String = str;
				m_Hash = MurmurHash2(m_String.c_str(), m_String.length(), 0x34123549);
			}
			else
			{
				m_String = "";
				m_Hash = 0;
			}
		}

		operator THash (){return m_Hash;}
		operator const char *(){return m_String.c_str();}
		const char * c_str() const {return m_String.c_str();}

		bool operator == (String b){return m_Hash == b.m_Hash;}
		bool operator == (const char *pString){return m_String.compare(pString) == 0;}
		bool operator == (const THash lhs){return m_Hash == lhs;}

		void operator = (const String &b){ m_Hash = b.m_Hash;m_String = b.m_String;}
		THash Hash() const { return m_Hash; }

	private:
		THash m_Hash;

		TString m_String;
	};
}
#endif //_MURMUR_HASH_




