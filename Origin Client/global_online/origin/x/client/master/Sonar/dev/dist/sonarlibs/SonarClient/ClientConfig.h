#pragma once

#include <SonarCommon/Common.h>

namespace sonar
{

	class ClientConfig
	{
	public:

		CString get(CString &name, CString &def);
		float get(CString &name, float def);
		int get(CString &name, int def);
		bool getBool(CString &name, bool def);

		void set(CString &name, CString &value);
		void set(CString &name, double value);
		void set(CString &name, bool value);
		void set(CString &name, int value);

		bool save(CString &filePath);
		bool load(CString &filePath);

	private:
		
		typedef SonarMap<CString, String> KeyValueMap;
		KeyValueMap m_kvMap;

	};



}