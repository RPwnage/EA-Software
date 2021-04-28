#pragma once

#include <SonarIpcClient/IpcAsyncClient.h>
#include <SonarClient/IClientCommands.h>
#include <SonarClient/IClientEvents.h>
#include <jlib/Spinlock.h>
#include <jlib/Condition.h>
#include <jlib/Thread.h>

namespace sonar
{
	class IpcClient : public IpcAsyncClient, public IClientCommands
	{
	public:
		IpcClient(void);
		~IpcClient(void);
		void update();

		void addEventListener(IClientEvents *listener);

		/* IClient */
		virtual bool connect(CString &token);
		virtual bool disconnect(sonar::CString &reasonType, sonar::CString &reasonDesc);
		virtual void muteClient(CString &userId, bool state);
		virtual void recordStroke();
		virtual void bindPushToTalkKey(CString &stroke, bool block);
		virtual void bindPlaybackMuteKey(CString &stroke, bool block);
		virtual void bindCaptureMuteKey(CString &stroke, bool block);
		virtual void setCaptureMode(CaptureMode mode);
		virtual void setPlaybackMute(bool state);
		virtual void setCaptureMute(bool state);
		virtual void setCaptureDevice (CString &deviceId);
		virtual void setPlaybackDevice (CString &deviceId);
		virtual void setLoopback(bool state);
		virtual void setAutoGain(bool state);
		virtual void setCaptureGain(int percent);
		virtual void setPlaybackGain(int percent);
		virtual void setVoiceSensitivity(int percent);
		virtual void setSoftPushToTalk(bool state);
		virtual CaptureMode getCaptureMode();
		virtual CString getPushToTalkKeyStroke();
		virtual CString getPlaybackMuteKeyStroke();
		virtual bool getPushToTalkKeyBlock();
		virtual bool getPlaybackMuteKeyBlock();
		virtual CString getCaptureMuteKeyStroke();
		virtual bool getCaptureMuteKeyBlock();
		virtual bool getPlaybackMute(void);
		virtual bool getCaptureMute(void);
		virtual PeerList getClients();
		virtual AudioDeviceList getCaptureDevices();
		virtual AudioDeviceList getPlaybackDevices();
		virtual AudioDeviceId getCaptureDevice();
		virtual AudioDeviceId getPlaybackDevice();
		virtual bool getLoopback(void);
		virtual bool getAutoGain(void);
		virtual int getCaptureGain(void);
		virtual int getPlaybackGain(void);
		virtual int getVoiceSensitivity(void);
		virtual bool getSoftPushToTalk(void);
		virtual bool isConnected(void);
		virtual bool isDisconnected(void);
		virtual CString getUserId();
		virtual CString getUserDesc();
		virtual CString getChannelId();
		virtual CString getChannelDesc();
		virtual int getChid(void);
		virtual Token getToken(void);

	private:
		void evtStrokeRecordedHandler(IpcMessage &msg);
		void evtCaptureModeChangedHandler(IpcMessage &msg);
		void evtPlaybackMuteKeyChangedHandler(IpcMessage &msg);
		void evtCaptureMuteKeyChangedHandler(IpcMessage &msg);
		void evtPushToTalkKeyChangedHandler(IpcMessage &msg);
		void evtConnectedHandler(IpcMessage &msg);
		void evtDisconnectedHandler(IpcMessage &msg);
		void evtShutdownHandler(IpcMessage &msg);
		void evtClientJoinedHandler(IpcMessage &msg);
		void evtClientPartedHandler(IpcMessage &msg);
		void evtClientMutedHandler(IpcMessage &msg);
		void evtClientTalkingHandler(IpcMessage &msg);
		void evtCaptureAudioHandler(IpcMessage &msg);
		void evtPlaybackMuteChangedHandler(IpcMessage &msg);
		void evtCaptureMuteChangedHandler(IpcMessage &msg);
		void evtCaptureDeviceChangedHandler(IpcMessage &msg);
		void evtPlaybackDeviceChangedHandler(IpcMessage &msg);
		void evtLoopbackChangedHandler(IpcMessage &msg);
		void evtAutoGainChangedHandler(IpcMessage &msg);
		void evtCaptureGainChangedHandler(IpcMessage &msg);
		void evtPlaybackGainChangedHandler(IpcMessage &msg);
		void evtVoiceSensitivityChangedHandler(IpcMessage &msg);

		CString postMessage(IpcMessage &msg);
		IpcMessage waitForReply(CString &id);
		IpcMessage postAndWait(IpcMessage &msg);

		static void *StaticThreadProc(void *arg);
		void threadProc(void);
		
		bool m_isRunning;

		jlib::Spinlock m_lock;
		jlib::Condition m_replyCond;
		IpcMessage m_replyMsg;
		SonarDeque<IpcMessage> m_inQueue;
		jlib::Thread m_thread;
		
		typedef void (IpcClient::*MessageHandler)(IpcMessage &msg);
		typedef SonarMap<const String, MessageHandler> MessageHandlerMap;
		static MessageHandlerMap s_messageHandlers;

		typedef SonarVector<IClientEvents *> EventListeners;
		EventListeners m_listeners;
	protected:
		virtual void onEvent(const IpcMessage &msg);
		virtual void onReply(const IpcMessage &msg);
	};


}
