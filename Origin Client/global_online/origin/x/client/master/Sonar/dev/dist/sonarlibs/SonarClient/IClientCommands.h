#pragma once

#include <SonarClient/ClientTypes.h>
#include <SonarCommon/Token.h>

namespace sonar
{
	class IClientCommands
	{
	public:

		/*
		Connects a not previously connected client to a voice server

		Arguments:
		token - A SONAR2 token

		Event:
		Fires IClientEvents::evtConnected once completed

		Result:
		true - If the initial connection attempt was successful
		false - Possibly token malformed or client is already connected
		*/
		virtual bool connect(CString &token) = 0;

		/*
		Disconnects a not previosly disconnected client from a voice server

		Arguments:
		reasonType - See reasonType
		reasonDesc - See reasonDesc

		Result:
		true - If the disconnect was sent successfuly
		false - Client is already disconnected
		*/	
		virtual bool disconnect(sonar::CString &reasonType, sonar::CString &reasonDesc) = 0;

		/*
		Sets the mute state for a remote client

		Event:
		Fires IClientEvents::evtDisconnected once completed

		Arguments: 
		userId - userId of remote client
		state - state of mute
		*/
		virtual void muteClient(CString &userId, bool state) = 0;

		/*
		Initiates the recording of a key stroke, used for binding keys

		Event:
		Fires IClientEvents::evtStrokeRecorded once completed
		*/
		virtual void recordStroke() = 0;

		/*
		Binds push-to-talk to the given stroke. 
		Unbinds the current stroke if present.
		Use getPushToTalkKeyStroke to validate if bind was successful

		Event:
		Fires IClientEvents::evtPushToTalkKeyChanged to notify

		Arguments:
		stroke - The key stroke to bind, may be combination of up to 4 keys
		block - Specifies if the stroke press should be prevented from reaching other applications in the operating system
		*/
		virtual void bindPushToTalkKey(CString &stroke, bool block) = 0;

		/*
		Binds playback mute to the given stroke. 
		Unbinds the current stroke if present.
		Use getPlaybackMuteStroke to validate if bind was successful

		Event:
		Fires IClientEvents::evtPlaybackMuteKeyChanged to notify

		Arguments:
		stroke - The key stroke to bind, may be combination of up to 4 keys
		block - Specifies if the stroke press should be prevented from reaching other applications in the operating system
		*/
		virtual void bindPlaybackMuteKey(CString &stroke, bool block) = 0;


		/*
		Binds capture mute to the given stroke. 
		Unbinds the current stroke if present.
		Use getCaptureMuteStroke to validate if bind was successful

		Event:
		Fires IClientEvents::evtCaptureMuteKeyChanged to notify

		Arguments:
		stroke - The key stroke to bind, may be combination of up to 4 keys
		block - Specifies if the stroke press should be prevented from reaching other applications in the operating system
		*/
		virtual void bindCaptureMuteKey(CString &stroke, bool block) = 0;


		/*
		Changes the capture mode. 

		Event:
		Fires IClientEvents::evtCaptureModeChanged to notify

		Arguments:
		mode - See CaptureMode enum
		*/
		virtual void setCaptureMode(CaptureMode mode) = 0;

		/*
		Changes the playback mute state effectivly silencing everyone in the channel

		Event:
		Fires IClientEvents::evtPlaybackMuteChanged to notify

		Arguments:
		state - State of mute
		*/
		virtual void setPlaybackMute(bool state) = 0;

		/*
		Changes the capture mute state preventing the channel from hearing the local client

		Event:
		Fires IClientEvents::evtCaptureMuteChanged to notify

		Arguments:
		state - State of mute
		*/
		virtual void setCaptureMute(bool state) = 0;

		/*
		Changes the current capture audio device. A list of available devices may be queried using getCaptureDevices()

		Event:
		Fires IClientEvents::evtCaptureDeviceChanged to notify

		Arguments:
		deviceId - Id of device, usually GUID
		*/
		virtual void setCaptureDevice (CString &deviceId) = 0;

		/*
		Changes the current playback audio device. A list of available devices may be queried using getPlaybackDevices()

		Event:
		Fires IClientEvents::evtPlaybackDeviceChanged to notify

		Arguments:
		deviceId - Id of device, usually GUID
		*/
		virtual void setPlaybackDevice (CString &deviceId) = 0;

		/*
		Enables direct loopback between capture and playback for users to try out microphone configuration

		Event:
		Fires IClientEvents::evtLoopbackChanged to notify

		Arguments:
		state - State of loopback
		*/
		virtual void setLoopback(bool state) = 0;

		/*
		Enables auto gain of capture/microphone input. This option effectivly disables any effect of capture gain and voice sensitivity
		
		Event:
		Fires IClientEvents::evtAutoGainChanged to notify

		Arguments:
		state - state of auto gain
		*/
		virtual void setAutoGain(bool state) = 0;

		/*
		Sets the capture gain as a percentage integer. If auto gain is enabled this parameter has no effect
		
		Event:
		Fires IClientEvents::evtCaptureGainChanged to notify

		Arguments:
		percent - Percent gain, in range of 0-100
		*/
		virtual void setCaptureGain(int percent) = 0;

		/*
		Sets the playback gain as a percentage integer
		
		Event:
		Fires IClientEvents::evtPlaybackGainChanged to notify

		Arguments:
		percent - Percent gain, in range of 0-100
		*/
		virtual void setPlaybackGain(int percent) = 0;

		/*
		Sets the voice sensitivity threshold as a percentage integer.	Higher threshold means more amplitude is required to trigger transmit.
		This parameter has no effect is auto gain is enabled  
		
		Event:
		Fires IClientEvents::evtVoiceSensitivityChanged to notify

		Arguments:
		percent - Percent threshold, in range of 0-100
		*/
		virtual void setVoiceSensitivity(int percent) = 0;


		/*
		Sets the state of soft push-to-talk. This can be used to trigger transmission in push-to-talk mode when the internal input system of the Client is unavailable.

		Events:
		Indirectly fires IClientEvents::evtCaptureAudio with vaState set to true if active 
		
		Arguments:
		state - State of soft push-to-talk
		*/
		virtual void setSoftPushToTalk(bool state) = 0;


		/*
		Retrieves the current capture mode 

		Returns:
		Current capture mode

		*/
		virtual CaptureMode getCaptureMode() = 0;


		/*
		Retrieves the stroke bound for the push-to-talk key

		Returns:
		Current stroke
		*/
		virtual CString getPushToTalkKeyStroke() = 0;

		/*
		Retrieves the stroke bound for playback mute key

		Returns:
		Current stroke
		*/
		virtual CString getPlaybackMuteKeyStroke() = 0;

		/*
		Returns the block setting for the push-to-talk key 

		Returns:
		true - if stroke blocks the key press
		false - if it does not
		*/
		virtual bool getPushToTalkKeyBlock() = 0;

		/*
		Returns the block setting for the playback mute key 

		Returns:
		true - if stroke blocks the key press
		false - if it does not
		*/
		virtual bool getPlaybackMuteKeyBlock() = 0;


		/*
		Retrieves the stroke bound for capture mute key

		Returns:
		Current stroke
		*/
		virtual CString getCaptureMuteKeyStroke() = 0;

		/*
		Returns the block setting for the capture mute key 

		Returns:
		true - if stroke blocks the key press
		false - if it does not
		*/
		virtual bool getCaptureMuteKeyBlock() = 0;


		/*
		Returns the state of the playback mute 

		Returns:
		true - if playback is muted
		false - if it's not
		*/
		virtual bool getPlaybackMute(void) = 0;

		/*
		Returns the state of the capture mute 

		Returns:
		true - if capture is muted
		false - if it's not
		*/
		virtual bool getCaptureMute(void) = 0;

		/*
		Returns all clients in the current channel

		Returns:
		A list of Peer objects 
		*/
		virtual PeerList getClients() = 0;

		/*
		Returns a list of available capture devices

		Returns:
		A list of AudioDeviceId objects
		*/
		virtual AudioDeviceList getCaptureDevices() = 0;

		/*
		Returns a list of available playback devices

		Returns:
		A list of AudioDeviceId objects
		*/
		virtual AudioDeviceList getPlaybackDevices() = 0;

		/*
		Returns the current capture device identifier

		Returns:
		Identifier of capture device as AudioDeviceId
		*/
		virtual AudioDeviceId getCaptureDevice() = 0;

		/*
		Returns the current playback device identifier

		Returns:
		Identifier of playback device as AudioDeviceId
		*/
		virtual AudioDeviceId getPlaybackDevice() = 0;

		/*
		Returns the loopback state

		Returns:
		true - if loopback is enabled
		false - if it's not
		*/
		virtual bool getLoopback(void) = 0;

		/*
		Returns the auto gain state

		Returns:
		true - if auto gain is enabled
		false - if it's not
		*/
		virtual bool getAutoGain(void) = 0;

		/*
		Returns the capture gain

		Returns:
		Gain as integer in percent (0-100)
		*/
		virtual int getCaptureGain(void) = 0;

		/*
		Returns the playback gain

		Returns:
		Gain as integer in percent (0-100)
		*/
		virtual int getPlaybackGain(void) = 0;

		/*
		Returns the voice sensitivity threshold for voice activation

		Returns:
		Threshold as integer in percent (0-100)
		*/
		virtual int getVoiceSensitivity(void) = 0;

		/*
		Returns the state of the soft push-to-talk

		Returns:
		true - if soft push-to-talk is enabled
		false - if it's not
		*/
		virtual bool getSoftPushToTalk(void) = 0;

		/*
		Connected state of connection.

		Returns:
		true - if the connection is fully established
		false - if disconnected or not fully connected
		*/
		virtual bool isConnected(void) = 0;

		/*
		Disconnected state of connection.

		Returns:
		true - if the connection is fully disconnected
		false - if connected or not fully disconnected
		*/
		virtual bool isDisconnected(void) = 0;

		/*
		UserId of the local user 

		Returns:
		String 
		*/
		virtual CString getUserId() = 0;

		/*
		UserDesc of the local user 

		Returns:
		String 
		*/
		virtual CString getUserDesc() = 0;

		/*
		ChannelId of the current channel 

		Returns:
		String 
		*/
		virtual CString getChannelId() = 0;

		/*
		ChannelDesc of the current channel 

		Returns:
		String 
		*/
		virtual CString getChannelDesc() = 0;

		/*
		Returns CHID (user CHannel IDentifier)

		Returns:
		Integer
		*/
		virtual int getChid(void) = 0;

		/*
		Returns the current token in use

		Returns:
		Token object
		*/
		virtual Token getToken(void) = 0;
		
	};
}