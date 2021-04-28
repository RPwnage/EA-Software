#pragma once

#include <SonarIpcCommon/SocketConnection.h>
#include <SonarIpcHost/IpcHost.h>

namespace sonar
{
	class IpcHost;

	class InboundConnection : public SocketConnection
	{
	public:
		InboundConnection (IpcHost &host);

	protected:
		virtual void onMessage(IpcMessage &msg);

	private:
		void connectHandler(IpcMessage &msg);
		void disconnectHandler(IpcMessage &msg);
		void muteClientHandler(IpcMessage &msg);
		void recordStrokeHandler(IpcMessage &msg);
		void bindPushToTalkKeyHandler(IpcMessage &msg);
		void bindPlaybackMuteKeyHandler(IpcMessage &msg);
		void bindCaptureMuteKeyHandler(IpcMessage &msg);
		void setCaptureModeHandler(IpcMessage &msg);
		void setPlaybackMuteHandler(IpcMessage &msg);
		void setCaptureMuteHandler(IpcMessage &msg);
		void setCaptureDeviceHandler(IpcMessage &msg);
		void setPlaybackDeviceHandler(IpcMessage &msg);
		void setLoopbackHandler(IpcMessage &msg);
		void setAutoGainHandler(IpcMessage &msg);
		void setCaptureGainHandler(IpcMessage &msg);
		void setPlaybackGainHandler(IpcMessage &msg);
		void setVoiceSensitivityHandler(IpcMessage &msg);
		void setSoftPushToTalkHandler(IpcMessage &msg);
		void getCaptureModeHandler(IpcMessage &msg);
		void getPushToTalkKeyStrokeHandler(IpcMessage &msg);
		void getPlaybackMuteKeyStrokeHandler(IpcMessage &msg);
		void getPushToTalkKeyBlockHandler(IpcMessage &msg);
		void getPlaybackMuteKeyBlockHandler(IpcMessage &msg);
		void getCaptureMuteKeyStrokeHandler(IpcMessage &msg);
		void getCaptureMuteKeyBlockHandler(IpcMessage &msg);
		void getPlaybackMuteHandler(IpcMessage &msg);
		void getCaptureMuteHandler(IpcMessage &msg);
		void getClientsHandler(IpcMessage &msg);
		void getCaptureDevicesHandler(IpcMessage &msg);
		void getPlaybackDevicesHandler(IpcMessage &msg);
		void getCaptureDeviceHandler(IpcMessage &msg);
		void getPlaybackDeviceHandler(IpcMessage &msg);
		void getLoopbackHandler(IpcMessage &msg);
		void getAutoGainHandler(IpcMessage &msg);
		void getCaptureGainHandler(IpcMessage &msg);
		void getPlaybackGainHandler(IpcMessage &msg);
		void getVoiceSensitivityHandler(IpcMessage &msg);
		void isConnectedHandler(IpcMessage &msg);
		void isDisconnectedHandler(IpcMessage &msg);
		void getUserIdHandler(IpcMessage &msg);
		void getUserDescHandler(IpcMessage &msg);
		void getChannelIdHandler(IpcMessage &msg);
		void getChannelDescHandler(IpcMessage &msg);
		void getChidHandler(IpcMessage &msg);
		void getTokenHandler(IpcMessage &msg);
		void getSoftPushToTalkHandler(IpcMessage &msg);
		void echoHandler(IpcMessage &msg);

		IpcHost &m_host;

		typedef void (InboundConnection::*MessageHandler)(IpcMessage &msg);
		typedef SonarMap<String, MessageHandler> MessageHandlerMap;

		static MessageHandlerMap s_messageHandlers;


	};


}