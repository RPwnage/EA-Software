#pragma once

#include <SonarCommon/Token.h>

namespace sonar
{

class TokenSigner
{
public:

	class KeyNotFoundException
	{
		//if this exception is thrown, run SonarMaster once to have it write the ~/sonarmaster.prv 
	};

	TokenSigner();
	~TokenSigner();
	CString sign(const Token &token);

	static CString base64Encode(const void *data, size_t cbData);

private:
	struct Private;
	Private *m_private;
};

}