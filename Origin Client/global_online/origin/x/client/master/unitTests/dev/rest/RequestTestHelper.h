#ifndef _REQUESTTESTHELPER_H
#define _REQUESTTESTHELPER_H

#include <QNetworkReply>
#include "services/rest/HttpServiceResponse.h"

namespace RequestTestHelper
{
	enum ReplyState
	{
		ReplySuccess,
		ReplyFailure
	};

	ReplyState waitForReply(QNetworkReply *reply);
	void waitForResp(Origin::Services::HttpServiceResponse *resp);
}

#endif