#include <TestUtils/BackendEvent.h>
#include <SonarIpcCommon/IpcMessage.h>

namespace sonar
{

BackendEvent::BackendEvent(const IpcMessage &_msg, EventType _type)
	: type(_type)
	, msg(_msg)
{
}

BackendEvent::~BackendEvent()
{
}

}
