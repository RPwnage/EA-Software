#pragma once
#include <SonarCommon/Common.h>

namespace sonar
{
	typedef String ByteString;

	class IpcMessage
	{
	public:
		typedef SonarVector<CString> Arguments;

		IpcMessage(CString &id, CString &name);
		IpcMessage(CString &name);
		IpcMessage();
		
		bool getBool(size_t iArg) const;
		int getInt(size_t iArg) const ;
		float getFloat(size_t iArg) const;
		CString &getString(size_t iArg) const;

		void writeBool(bool value);
		void writeInt(int value);
		void writeFloat(float value);
		void writeString(CString &value);

		CString &getId(void) const;
		void setId(CString &id);

		CString &getName(void) const;
		void setName(CString &name);
		const Arguments &getArguments(void) const;

		ByteString toByteString(CString &newId) const;
		ByteString toByteString() const;

		void setArgument(const Arguments &args);

	private:
		size_t m_readIndex;
		Arguments m_arguments;
		String m_id;
		String m_name;
	};

}