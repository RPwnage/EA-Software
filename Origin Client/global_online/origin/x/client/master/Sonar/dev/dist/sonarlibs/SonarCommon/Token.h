#pragma once
#include <SonarCommon/Common.h>

namespace sonar
{

class Token
{
public:
	Token(void);
	bool parse (CString &token);
	typedef SonarVector<String> ArgumentList;
	const ArgumentList &getArguments (void) const;
	CString toString() const;

public:
	void setVoipAddress (const CString &address);

	CString &getUserId (void) const;
	CString &getOperatorId (void) const;
	CString &getUserDesc (void) const;
	CString &getChannelId (void) const;
	CString &getChannelDesc (void) const;
	CString &getLocation (void) const;
	CString &getCreationTime (void) const;
	CString &getSign (void) const;
	CString &getControlAddress (void) const;
	CString &getVoipAddress (void) const;

private:
	ArgumentList m_args;
	
};

}
