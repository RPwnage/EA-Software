#pragma once

#include <SonarClient/ClientTypes.h>

namespace sonar
{
	class IClientEvents
	{
	public:

		/*
		A stroke has been recorded. Fired as an asynchronous IClientCommand::recordStroke call finishes

		Arguments:
		stroke - The stroke recorded or empty if recording was aborted by user (ESC was pressed)
		*/
		virtual void evtStrokeRecorded(CString &stroke) = 0;

		/*
		The capture mode has been changed. Fired when IClientCommand::setCaptureMode has been completed

		Arguments:
		mode - The new capture mode
		*/
		virtual void evtCaptureModeChanged(CaptureMode mode) = 0;

		/*
		The playback mute key been changed. Fired when IClientCommand::bindPlaybackMuteKey has been completed

		Arguments:
		stroke - The new stroke
		block - Blocking 
		*/
		virtual void evtPlaybackMuteKeyChanged(CString &stroke, bool block) = 0;


		/*
		The capture mute key been changed. Fired when IClientCommand::bindCaptureMuteKey has been completed

		Arguments:
		stroke - The new stroke
		block - Blocking 
		*/
		virtual void evtCaptureMuteKeyChanged(CString &stroke, bool block) = 0;

		/*
		The push-to-talk key been changed. Fired when IClientCommand::bindPushToTalkKey has been completed

		Arguments:
		stroke - The new stroke
		block - Blocking 
		*/
		virtual void evtPushToTalkKeyChanged(CString &stroke, bool block) = 0;

        /*
        Push-to-talk activation has changed to either active or not active

        Arguments:
        active - Is Push-to-talk now active?
        */
        virtual void evtPushToTalkActiveChanged(bool active) = 0;

		/*
		The connection has been established. Fired when IClientCommand::connect has been completed

		Arguments:
		chid - Client CHannel IDentifier
		channelId - Id of channel that has been joined
		channelDesc - Desc of channel thas has been joined
		*/
		virtual void evtConnected(int chid, CString &channelId, CString channelDesc) = 0;

		/*
		The connection is disconnected. Fired when IClientCommand::disconnect has been completed or when the connection has been lost for other reasons

		Arguments:
		reasonType - Type for the reason
		reasonDesc - Desc for th reason
		*/
		virtual void evtDisconnected(sonar::CString &reasonType, sonar::CString &reasonDesc) = 0;

		/*
		Not used
		*/
		virtual void evtShutdown(void) = 0;

		/*
		A client has joined the channel 
		
		Arguments: 
		client - PeerInfo structure 
		*/
		virtual void evtClientJoined (const PeerInfo &client) = 0;

		/*
		A client has left the channel
		
		Arguments: 
		client - PeerInfo structure 
		*/
		virtual void evtClientParted (const PeerInfo &client, sonar::CString &reasonType, sonar::CString &reasonDesc) = 0;

		/*
		A client has been muted or unmuted
		
		Arguments: 
		client - PeerInfo structure 
		*/
		virtual void evtClientMuted (const PeerInfo &client) = 0;

		/*
		A client has begun or stopped talking
		
		Arguments: 
		client - PeerInfo structure 
		*/
		virtual void evtClientTalking (const PeerInfo &client) = 0;

		/*
		An audio frame has been capture from the audio capture device.
		
		Arguments: 
		clip - Clipping has occured due to overdrive
		avgAmp - The average amplitude of this frame as positive float
		peakAmp - The peak amplitude of this frame as positive float
		vaState - Voice activation state, true if threshold is being triggered
		xmit - Transmitting, indicates if audio is being transmitted to the remote voice channel from the local client
		*/
		virtual void evtCaptureAudio (bool clip, float avgAmp, float peakAmp, bool vaState, bool xmit) = 0;

		/*
		State of playback mute has been changed. 
		
		Arguments: 
		state - State of the mute
		*/
		virtual void evtPlaybackMuteChanged (bool state) = 0;

		/*
		State of capture mute has been changed. 
		
		Arguments: 
		state - State of the mute
		*/
		virtual void evtCaptureMuteChanged (bool state) = 0;

		/*
		Current capture device has changed.
		
		Arguments: 
		deviceId - Id of the device
		deviceName - Name of the device (as localized by the operating system)
		*/
		virtual void evtCaptureDeviceChanged(CString &deviceId, CString &deviceName) = 0;

		/*
		Current playback device has changed.
		
		Arguments: 
		deviceId - Id of the device
		deviceName - Name of the device (as localized by the operating system)
		*/
		virtual void evtPlaybackDeviceChanged(CString &deviceId, CString &deviceName) = 0;

		/*
		Loopback state has changed.
		
		Arguments: 
		state - State of loopback
		*/
		virtual void evtLoopbackChanged (bool state) = 0;

		/*
		Auto gain has changed
		
		Arguments: 
		state - State of auto gain
		*/
		virtual void evtAutoGainChanged(bool state) = 0;

		/*
		Capture gain has changed
		
		Arguments: 
		percent - Gain 
		*/
		virtual void evtCaptureGainChanged (int percent) = 0;

		/*
		Playback gain has changed
		
		Arguments: 
		percent - Gain 
		*/
		virtual void evtPlaybackGainChanged (int percent) = 0;

		/*
		Voice sensitivity gain has changed
		
		Arguments: 
		percent - Gain 
		*/
		virtual void evtVoiceSensitivityChanged (int percent) = 0;

        /*
        Number of contiguous talk frames sent by client is over the limit (protocol::IN_TAKE_MAX_FRAMES)

        Arguments:
        over - over limit
        */
        virtual void evtTalkTimeOverLimit(bool over) = 0;

        /*
        No inactivity for 'interval' milliseconds

        Arguments:
        interval - milliseconds of inactivity
        */
        virtual void evtChannelInactivity(unsigned long interval) = 0;
    };
}