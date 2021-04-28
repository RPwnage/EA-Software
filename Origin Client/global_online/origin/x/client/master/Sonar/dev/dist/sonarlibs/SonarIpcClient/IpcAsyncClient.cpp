#include <SonarIpcClient/IpcAsyncClient.h>

namespace sonar
{

CString IpcAsyncClient::postMessage(IpcMessage &msg)
{
	CString id = generateId();
	msg.setId(id);
	send(msg);
	return id;
}

void IpcAsyncClient::onMessage(IpcMessage &msg)
{
	if (msg.getName() == "reply")
	{
		onReply(msg);
	}
	else
	if (msg.getName().find("evt") == 0)
	{
		onEvent(msg);
	}
}

void IpcAsyncClient::onEvent(const IpcMessage &msg)
{
}

void IpcAsyncClient::onReply(const IpcMessage &msg)
{
}

}