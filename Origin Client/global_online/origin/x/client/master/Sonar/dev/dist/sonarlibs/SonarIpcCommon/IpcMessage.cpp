#include <SonarCommon/Common.h>
#include <SonarIpcCommon/IpcMessage.h>
#include <jlib/String.h>

namespace sonar
{

IpcMessage::IpcMessage(CString &id, CString &name)
	: m_id(id)
	, m_name(name)
	, m_readIndex(0)
{
}

IpcMessage::IpcMessage(CString &name)
	: m_name(name)
	, m_readIndex(0)
{
}

IpcMessage::IpcMessage()
	: m_readIndex(0)
{
}

bool IpcMessage::getBool(size_t iArg) const
{
	CString &value = m_arguments[iArg];

	if (value == "true")
	{
		return true;
	}

	return false;
}

int IpcMessage::getInt(size_t iArg) const
{
	CString &value = m_arguments[iArg];
	char *end;
	return strtol(value.c_str(), &end, 10);
}

float IpcMessage::getFloat(size_t iArg) const
{
	CString &value = m_arguments[iArg];
	char *end;
	return (float) strtod(value.c_str(), &end);
}

CString &IpcMessage::getString(size_t iArg) const
{
	return m_arguments[iArg];
}

void IpcMessage::writeBool(bool value)
{
	m_arguments.push_back(value ? "true" : "false");
}

void IpcMessage::writeInt(int value)
{
	StringStream ss;
	ss << value;
	m_arguments.push_back(ss.str());
}

void IpcMessage::writeFloat(float value)
{
	StringStream ss;
	ss << value;
	m_arguments.push_back(ss.str());
}

void IpcMessage::writeString(CString &value)
{
	m_arguments.push_back(value);
}

CString &IpcMessage::getId(void) const
{
	return m_id;
}

void IpcMessage::setId(CString &id)
{
	m_id = id;
}

CString &IpcMessage::getName(void) const 
{
	return m_name;
}

void IpcMessage::setName(CString &name)
{
	m_name = name;
}

ByteString IpcMessage::toByteString(CString &newId) const
{
	StringStream ss;

	ss << newId;
	ss << "\t";
	ss << m_name;

	for (Arguments::const_iterator iter = m_arguments.begin(); iter != m_arguments.end(); iter ++)
	{
		ss << "\t";
		ss << *iter;
	}

	ss << "\n";

	return ss.str();
}



ByteString IpcMessage::toByteString() const
{
	return toByteString(getId());
}

void IpcMessage::setArgument(const Arguments &args)
{
	m_arguments = args;
}

const IpcMessage::Arguments &IpcMessage::getArguments(void) const
{
	return m_arguments;
}


}