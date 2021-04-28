;(function($){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.views) { Origin.views = {}; }

var testingMicrophone = false;
var gAudioInputDevices, gAudioOutputDevices;

var VoiceView = function()
{
	$("#voice-leHotKeys").keyup = this.setHotKeyInputFocus(true);
    $("#voice-leHotKeys").focusin = this.setHotKeyInputFocus(true);
    $("#voice-leHotKeys").focusout = this.setHotKeyInputFocus(false);

    // Set text
	$("#page_title").text(tr('ebisu_client_settings'));
    $("#voice-lblVoiceChatIndicatorEnabled").text(tr('ebisu_client_enable_voice_chat_indicator').replace("%1", tr("ebisu_client_igo_name")));
    $("#voice-lblIncomingVoiceChatRequestsDeclined").text(tr('ebisu_client_decline_voice_chat_requests'));
    $("#voice-lblVoiceChatConflictNoWarn").text(tr('ebisu_client_no_warn_voice_chat_conflict'));

    $("#voice-lblHotKeys").text(tr('ebisu_client_push_to_talk_shortcut'));
    $("#voice-lblPushToTalkDesc").text(tr('ebisu_client_push_to_talk_description'));
    $("#voice-lblVoiceActivationDesc").text(tr('ebisu_client_voice_activation_description'));
    $("#voice-lblInputLevel").text(tr('ebisu_client_voice_input_level'));
    $("#voice-lblThreshold").text(tr('ebisu_client_voice_threshold'));

    var $sliderMicrophone = $.otkSliderCreate($("#voice-microphone-slider"),
    {
        value: .75,
        pointerDirection: "none" // Direction can be "none", "up" or "down"
    });

    var $sliderSpeaker = $.otkSliderCreate($("#voice-speaker-slider"),
    {
        value: .45,
        pointerDirection: "none" // Direction can be "none", "up" or "down"
    });

    var $sliderActivation = $.otkSliderCreate($("#voice-activation-slider"),
    {
        value: .65,
        pointerDirection: "up" // Direction can be "none", "up" or "down"
    });

    var $levelIndicatorVoiceActivation = $.otkLevelIndicatorCreate($("#voice-activation-level-indicator"),
    {
      value: 0
    });

    // Voice activation audio level indicator
    originVoice.voiceLevel.connect(function(level) {
        if(testingMicrophone || originVoice.isInVoice)
            $levelIndicatorVoiceActivation.otkLevelIndicator("setValue", level / 100);
    });

    // Voice activation microphone
    originVoice.overThreshold.connect(function() {
        if(testingMicrophone || originVoice.isInVoice)
            $("#voice-activation-microphone").addClass('overThreshold');
    });

    originVoice.underThreshold.connect(function() {
        if(testingMicrophone || originVoice.isInVoice)
            $("#voice-activation-microphone").removeClass('overThreshold');
    });

	document.onclick = function()
	{
		// We need to deselect the Push-to-Talk Hotkey if the user presses the "Restore Default"
		// button. However, focusout isn't working. So easy solution (might not be the best)
		// is so compare what the current object in focus is.

		var clickElementID = window.event.srcElement.id;
		if(clickElementID === "voice-leHotKeys")
		{
			Origin.views.voice.setHotKeyInputFocus(true);
		}
        else if(clickElementID === "voice-btnRestore")
		{
			Origin.views.voice.setHotKeyInputFocus(false);
			clientSettings.restoreDefaultHotkey("PushToTalkKey", "PushToTalkKeyString");
		}
		else
		{
			Origin.views.voice.setHotKeyInputFocus(false);
		}
	}

    // Audio device added
    originVoice.deviceAdded.connect(function(deviceName) {
        Origin.views.voice.updateDeviceListAndSelection();
    });

    // Audio device removed
    originVoice.deviceRemoved.connect(function() {
        Origin.views.voice.updateDeviceListAndSelection();
    });

    // Default audio device changed
    originVoice.defaultDeviceChanged.connect(function() {
        Origin.views.voice.updateDeviceListAndSelection();
    });

	// WinXP: Audio device changed
    originVoice.deviceChanged.connect(function() {
        Origin.views.voice.updateDeviceListAndSelection();
    });
	
    // Settings voice connection started
    originVoice.voiceConnected.connect(function() {
        Origin.views.voice.enableInputLevelAndThreshold(true);
    });

    // Settings voice connection ended
    originVoice.voiceDisconnected.connect(function() {
        Origin.views.voice.enableInputLevelAndThreshold(false);
    });

    // Enable Test Microphone
    originVoice.enableTestMicrophone.connect(function(resetButton) {
		Origin.views.voice.updateTestMicrophone(true, resetButton);
    });

    // Disable Test Microphone
    originVoice.disableTestMicrophone.connect(function(resetButton) {
		Origin.views.voice.updateTestMicrophone(false, resetButton);
    });

    // Clear level indicator
    originVoice.clearLevelIndicator.connect(function() {
        Origin.views.voice.clearLevelIndicator();
    });

    // Online / offline
    originSocial.connection.changed.connect(function() {
    });
    
    // Input device changed
    $("#voice-ddInputDevices").change(function() {
        originVoice.changeInputDevice($("#voice-ddInputDevices").val());
        clientSettings.writeSetting("VoiceInputDevice", $("#voice-ddInputDevices").val());
    })
    .on('setupDeviceListInput_', function() {
        Origin.views.voice.setupDeviceListInput(gAudioInputDevices);
        $.fn.originUxElements("dropdown-repaint");
        $("#voice-ddInputDevices").trigger('setupDeviceSelectionInput_');
    })
    .on('setupDeviceSelectionInput_', function() {
        var selectedInputDevice = originVoice.selectedAudioInputDevice;
        $("#voice-ddInputDevices").prop("value", selectedInputDevice);
        $.fn.originUxElements("dropdown-repaint");
    });

    // Output device changed
    $("#voice-ddOutputDevices").change(function() {
        originVoice.changeOutputDevice($("#voice-ddOutputDevices").val());
        clientSettings.writeSetting("VoiceOutputDevice", $("#voice-ddOutputDevices").val());
    })
    .on('setupDeviceListOutput_', function() {
        Origin.views.voice.setupDeviceListOutput(gAudioOutputDevices);
        $.fn.originUxElements("dropdown-repaint");
        $("#voice-ddOutputDevices").trigger('setupDeviceSelectionOutput_');
    })
    .on('setupDeviceSelectionOutput_', function() {
        var selectedOutputDevice = originVoice.selectedAudioOutputDevice;
        $("#voice-ddOutputDevices").prop("value", selectedOutputDevice);
        $.fn.originUxElements("dropdown-repaint");
    });

    // Microphone level slider
    $("#voice-microphone-slider").on('valueChanged', function(e, value)
    {
        clientSettings.writeSetting("MicrophoneGain", parseInt(value*100));

        $("#voice-microphone-plus").removeClass("maxVolume");
        if (value*100 >= 100)
        {
            $("#voice-microphone-plus").addClass("maxVolume");
        }
    });

    // Incoming volume slider
    $("#voice-speaker-slider").on('valueChanged', function(e, value)
    {
        clientSettings.writeSetting("SpeakerGain", parseInt(value*100));

        $("#voice-speaker-plus").removeClass("maxVolume");
        if (value*100 >= 100)
        {
            $("#voice-speaker-plus").addClass("maxVolume");
        }
    });

     // Push to Talk enabled
    $("#voice-rbPushToTalk").on('change', function(evt)
    {
        clientSettings.writeSetting("EnablePushToTalk", this.checked);
    });

     // Transmit audio
    $("#voice-rbVoiceActivation").on('change', function(evt)
    {
        clientSettings.writeSetting("EnablePushToTalk", !this.checked);
    });

    // Voice activation slider
    $("#voice-activation-slider").on('valueChanged', function(e, value)
    {
        clientSettings.writeSetting("VoiceActivationThreshold", parseInt(value*100));
    });

    $("#voice-btnTest").on("click", function(evt)
    {
        if ($("#voice-btnTest").hasClass("disabled") === false)
        {
            if (!testingMicrophone)
            {
                Origin.views.voice.updateTestMicrophone(false, false); // prevent spam

                window.setTimeout(function () {
                   originVoice.testMicrophoneStart();
                }, 0);
                $("#voice-btnTest").text(tr('ebisu_client_settings_voice_test_button_stop'));
                testingMicrophone = true;
            }
            else
            {
                Origin.views.voice.updateTestMicrophone(false, false); // prevent spam
                Origin.views.voice.clearLevelIndicator();

                window.setTimeout(function () {
                   originVoice.testMicrophoneStop();
                }, 0);
                $("#voice-btnTest").text(tr('ebisu_client_test_mic'));
                testingMicrophone = false;
            }
        }
    });

    // Voice Chat Indicator
    $("#voice-chkVoiceChatIndicatorEnabled").on('click', function(evt)
    {
        var settingVal = 0;
        // These numbers correlate to an enum OriginToastManager::ToastSetting::AlertMethod
        // 0 == TOAST_DISABLED
        // 2 == TOAST_ALERT_ONLY
        settingVal = this.checked ? 2 : 0;
        clientSettings.writeSetting("EnableVoiceChatIndicator", settingVal);
    });

    // Incoming Voice Chat Requests
    $("#voice-chkIncomingVoiceChatRequestsDeclined").on('click', function(evt)
    {
        clientSettings.writeSetting("DeclineIncomingVoiceChatRequests", this.checked);
    });

    // Voice Chat Conflict Warning
    $("#voice-chkVoiceChatConflictNoWarn").on('click', function(evt)
    {
        clientSettings.writeSetting("NoWarnAboutVoiceChatConflict", this.checked);
    });

    clientSettings.updateSettings.connect($.proxy(this, 'onUpdateSettings'));

    this.onUpdateSettings();

    this.setupDeviceListAndSelection('input');
    this.setupDeviceListAndSelection('output');

    // only done on init
    var enable = originSocial.connection.established && (originVoice.audioInputDevices.length > 0) && !originVoice.isInVoice;
	this.updateTestMicrophone(enable, true);
    this.enableInputLevelAndThreshold(originVoice.isInVoice);

    // stop the voice channel when the user navigates out of the settings page
    originPageVisibility.visibilityChanged.connect($.proxy(this, 'onVisibilityChanged'));
    this.onVisibilityChanged();
};

VoiceView.prototype.enableInputLevelAndThreshold = function(enable)
{
    $("#voice-activation-slider").toggleClass("disabled", !enable);
    $("#voice-activation-microphone").toggleClass("disabled", !enable);

    $("#voice-activation-level-indicator").toggleClass("disabled", !enable);
    $("#voice-activation-level-indicator").otkLevelIndicator("setValue", 0);

    if(!enable)
        $("#voice-activation-microphone").removeClass('overThreshold');
}

VoiceView.prototype.updateTestMicrophone = function( enableButton, resetButton )
{
	if(enableButton)
	{
	    $("#voice-btnTest").removeClass("disabled");
	}
	else
	{
        $("#voice-btnTest").addClass("disabled");
	}

    if(resetButton)
    {
        $("#voice-btnTest").text(tr('ebisu_client_test_mic'));
        testingMicrophone = false;
    }
}

VoiceView.prototype.clearLevelIndicator = function()
{
    $("#voice-activation-level-indicator").otkLevelIndicator("setValue", 0);
	$("#voice-activation-microphone").removeClass('overThreshold');
}

VoiceView.prototype.onVisibilityChanged = function()
{
    if(originPageVisibility.hidden === true)
    {
        window.setTimeout(originVoice.setInVoiceSettings(false), 0);
    }
    else
    {
        window.setTimeout(originVoice.setInVoiceSettings(true), 0);
    }
}

VoiceView.prototype.updateDeviceListAndSelection = function()
{
    Origin.views.voice.setupDeviceListAndSelection('input');
    Origin.views.voice.setupDeviceListAndSelection('output');
}

VoiceView.prototype.setupDeviceListInput = function(audioInputDevices)
{
    var AudioInputDevices, device, option, i, numLang;

    function compare(a,b)
    {
        if (a.string < b.string)  return -1;
        if (a.string > b.string)  return 1;
        return 0;
    }

    $("#voice-ddInputDevices").empty();

    if (audioInputDevices.length)
    {
        AudioInputDevices = [];
        device;
        for(i = 0, numLang = audioInputDevices.length; i < numLang; i++)
        {
            device = audioInputDevices[i];
            AudioInputDevices.push({ id: device, string: device });
        }
        AudioInputDevices.sort(compare);

        option = "";
        for(i = 0, numLang = audioInputDevices.length; i < numLang; i++)
        {
            option = ('<option value="%0">%1</option>').replace("%0", AudioInputDevices[i].id);
            option = option.replace("%1", AudioInputDevices[i].string);
            $("#voice-ddInputDevices").append(option);
        }
    }
    else
    {
        // no devices found, so add a 'None Found' option
        $("#voice-ddInputDevices").append('<option value="' + tr('ebisu_client_none_found') + '">' + tr('ebisu_client_none_found') + '</option>');
    }
}

VoiceView.prototype.setupDeviceListOutput = function(audioOutputDevices)
{
    var AudioOutputDevices, device, option, i, numLang;

    function compare(a,b)
    {
        if (a.string < b.string)  return -1;
        if (a.string > b.string)  return 1;
        return 0;
    }

    $("#voice-ddOutputDevices").empty();

    if (audioOutputDevices.length)
    {
        AudioOutputDevices = [];
        device;
        for(i = 0, numLang = audioOutputDevices.length; i < numLang; i++)
        {
            device = audioOutputDevices[i];
            AudioOutputDevices.push({ id: device, string: device });
        }
        AudioOutputDevices.sort(compare);

        option = "";
        for(i = 0, numLang = audioOutputDevices.length; i < numLang; i++)
        {
            option = ('<option value="%0">%1</option>').replace("%0", AudioOutputDevices[i].id);
            option = option.replace("%1", AudioOutputDevices[i].string);
            $("#voice-ddOutputDevices").append(option);
        }
    }
    else
    {
        // no devices found, so add a 'None Found' option
        $("#voice-ddOutputDevices").append('<option value="' + tr('ebisu_client_none_found') + '">' + tr('ebisu_client_none_found') + '</option>');
    }
}

VoiceView.prototype.setupDeviceListAndSelection = function(deviceType)
{
    if( deviceType === "input" )
    {
        window.setTimeout(function() {
            gAudioInputDevices = originVoice.audioInputDevices;
            $("#voice-ddInputDevices").trigger('setupDeviceListInput_');
        }, 0);
    }
    else if( deviceType === "output" )
    {
        window.setTimeout(function() {
            gAudioOutputDevices = originVoice.audioOutputDevices;
            $("#voice-ddOutputDevices").trigger('setupDeviceListOutput_');
        }, 0);
    }
}

VoiceView.prototype.onUpdateSettings = function(settingName)
{
    var pttString, pttButton, pushToTalk;

    // Update the device dropdowns
    if (settingName === "VoiceInputDevice")
    {
        $("#voice-ddInputDevices").trigger('setupDeviceSelectionInput_');
    }
    else if (settingName === "VoiceOutputDevice")
    {
        $("#voice-ddOutputDevices").trigger('setupDeviceSelectionOutput_');
    }

    pushToTalk = clientSettings.readSetting("EnablePushToTalk");

    $("#voice-rbPushToTalk").prop("checked", pushToTalk);

    $("#voice-rbVoiceActivation").prop("checked", !pushToTalk);
    $("#voice-rbPushToTalk").prop("checked", pushToTalk);

    if (pushToTalk)
    {
        $("#captureMode").removeClass('voiceActivated');
        $("#captureMode").addClass('pushToTalk');
        $('#voice-microphone-slider').removeClass('disabled');
    }
    else
    {
        $("#captureMode").removeClass('pushToTalk');
        $("#captureMode").addClass('voiceActivated');
    }

    $("#voice-activation-level-indicator").otkLevelIndicator("setValue", 0);

    pttString = clientSettings.readSetting("PushToTalkKeyString");
    if (pttString.indexOf("mouse") != -1)
    {
        pttButton = pttString.match(/\d+/);
        pttString = tr('ebisu_string_mouse_button_voip').replace("%1", pttButton);
    }
    $('#voice-leHotKeys').val(pttString);

    $("#voice-microphone-slider").otkSlider("setValue", clientSettings.readSetting("MicrophoneGain") / 100);

    $("#voice-speaker-slider").otkSlider("setValue", clientSettings.readSetting("SpeakerGain") / 100);

    $("#voice-activation-slider").otkSlider("setValue", clientSettings.readSetting("VoiceActivationThreshold") / 100);

    $("#voice-chkVoiceChatIndicatorEnabled").prop("checked", clientSettings.readSetting("EnableVoiceChatIndicator"));

    $("#voice-chkIncomingVoiceChatRequestsDeclined").prop("checked", clientSettings.readSetting("DeclineIncomingVoiceChatRequests"));

    $("#voice-chkVoiceChatConflictNoWarn").prop("checked", clientSettings.readSetting("NoWarnAboutVoiceChatConflict"));
}

VoiceView.prototype.setHotKeyInputFocus = function(hasFocus)
{
	if(hasFocus)
	{
		$("#voice-leHotKeys").select();
		if(window.helper) window.helper.pushToTalkHotKeyInputHasFocus(true);
	}
	else
	{
		if (window.getSelection)
        {
            window.getSelection().empty();
        }
        if(window.helper) window.helper.pushToTalkHotKeyInputHasFocus(false);
	}
}

// Expose to the world
Origin.views.voice = new VoiceView();

})(jQuery);
