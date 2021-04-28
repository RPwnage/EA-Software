#include <QCoreApplication>
#include <QSignalSpy>

#include "../RequestTestHelper.h"

RequestTestHelper::ReplyState RequestTestHelper::waitForReply(QNetworkReply *reply)
{
	QSignalSpy errorSpy(reply, SIGNAL(error(QNetworkReply::NetworkError)));
	QSignalSpy finishedSpy(reply, SIGNAL(finished()));

	while(finishedSpy.empty())
	{
		QCoreApplication::processEvents();
	}

	if (errorSpy.length())
	{
		return RequestTestHelper::ReplyFailure;
	}
	else
	{
		return RequestTestHelper::ReplySuccess;
	}
}


void RequestTestHelper::waitForResp(Origin::Services::HttpServiceResponse *resp)
{
	QSignalSpy finishedSpy(resp, SIGNAL(finished()));
	while(finishedSpy.empty())
	{
		QCoreApplication::processEvents();
	}
}