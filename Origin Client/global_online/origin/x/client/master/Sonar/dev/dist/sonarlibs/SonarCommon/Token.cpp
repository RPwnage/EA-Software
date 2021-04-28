#include <SonarCommon/Token.h>
#include <SonarCommon/Common.h>
#include <sstream>

using namespace std;

namespace sonar
{

CString Token::toString() const
{
	String ret;

	ret += "SONAR2";
	ret += "|";
	ret += m_args[0];
	ret += "|";
	ret += m_args[1];
	ret += "|";
	ret += m_args[2];
	ret += "|";
	ret += m_args[3];
	ret += "|";
	ret += m_args[4];
	ret += "|";
	ret += m_args[5];
	ret += "|";
	ret += m_args[6];
	ret += "|";
	ret += m_args[7];
	ret += "|";
	ret += m_args[8];
	ret += "|";
	ret += m_args[9];

	return ret;
}

bool Token::parse (CString &_token)
{
	if (_token.find_first_of ("SONAR2|") != 0)
	{
		return false;
	}

	TokenList slist = common::tokenize (_token, L'|');

	m_args = ArgumentList(slist.begin(), slist.end());

	if (m_args.size () < 11)
	{
		return false;
	}

	m_args.erase (m_args.begin ());

	return true;
}

Token::Token(void)
{
	m_args.resize (11);
}


void Token::setVoipAddress (CString &_address)
{
	m_args[1] = _address;
}

const Token::ArgumentList &Token::getArguments (void) const
{
	return m_args;
}

CString &Token::getControlAddress (void) const
{
	return m_args[0];
}

CString &Token::getVoipAddress (void) const
{
	return m_args[1];
}


CString &Token::getOperatorId (void) const 
{
	return m_args[2];
}

CString &Token::getUserId (void) const
{
	return m_args[3];
}

CString &Token::getUserDesc (void) const
{
	return m_args[4];
}

CString &Token::getChannelId (void) const
{
	return m_args[5];
}

CString &Token::getChannelDesc (void) const
{
	return m_args[6];
}

CString &Token::getLocation (void) const
{
	return m_args[7];
}

CString &Token::getCreationTime (void) const
{
	return m_args[8];
}

CString &Token::getSign (void) const
{
	return m_args[9];
}


}