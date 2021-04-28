/**
 * @file voice.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-settings-voice';
    var CLS_HKERROR3  = '.origin-settings-oig-hkerror3';
	var CLS_ERROR_ADJUST = "ui-errormsg-adjust";
	var CLS_NOERROR_ADJUST = "ui-noerrormsg-adjust";
	
    function OriginSettingsVoiceCtrl($scope, $timeout, UtilFactory, VoiceFactory) {
        var self = this;
        var $element, $micLevelElement,
            $micLevelBar0, $micLevelBar1, $micLevelBar2, $micLevelBar3, $micLevelBar4,
            $micLevelBar5, $micLevelBar6, $micLevelBar7, $micLevelBar8, $micLevelBar9,
            $micLevelBar10, $micLevelBar11, $micLevelBar12, $micLevelBar13, $micLevelBar14,
            $micLevelBar15, $micLevelBar16, $micLevelBar17, $micLevelBar18, $micLevelBar19,
            $micLevelBar20;

        var hotKeyAssignment = "",
            settingToDisplayMap = {};

        $scope.testingMicrophone = false;
		$scope.pTTKeyNoError = true;		

        var throttledDigestFunction = UtilFactory.throttle(function() {
            $timeout(function() {
                $scope.$digest();
            }, 0, false);
        }, 1000);

		function localize(defaultStr, key) {
		   return UtilFactory.getLocalizedStr(defaultStr, CONTEXT_NAMESPACE, key);
		} 
		
		
        $scope.mouseButtonLoc = localize($scope.mousebuttonstr, 'mousebuttonstr');

        function assignSettingToScope(prop) {
            return function(setting) {

                // special case for 'voice_chat_indicator'
                if (prop === 'voice_chat_indicator') {
                    setting = (setting !== 0);
                }

                // special case for 'no_warning_voice_conflict'
                if (prop === 'no_warning_voice_conflict') {
                    setting = (setting === false);
                }

                // special case for 'enable_push_to_talk'
                if (prop === 'enable_push_to_talk') {
                    // convert to boolean to string
                    setting = setting ? 'true' : 'false';
                }

                // special case for 'pushToTalkString'
                if (prop === 'pushToTalkString') {
                    if (setting.indexOf('mouse') !== -1) {
                        var pttButton = setting.match(/\d+/);
                        setting = $scope.mouseButtonLoc.replace("%number%", pttButton);
                    }
                }

                $scope[prop] = setting;
                throttledDigestFunction();
            };
        }

        this.updateTestMicrophone = function (enableLink, resetLink) {
            if(enableLink)
            {
                $scope.$testMicrophoneLink.removeClass('otka-disabled');
            } else {
                $scope.$testMicrophoneLink.addClass('otka-disabled');
            }

            if(resetLink)
            {
                $scope.testMicLinkStr = $scope.testMicLoc;
                $scope.testingMicrophone = false;
            }
        };

        this.setElement = function ($el) {
            $element = $el;
        };

        $scope.noneFoundLoc = localize($scope.nonefoundstr, 'nonefoundstr');

        function setupDeviceListAndSelection() {
            if (Origin.voice.supported()) {
                // get the input devices
                Origin.voice.audioInputDevices().then(function(devices) {
                    if (devices.length > 0) {
                        $scope.inputDevices = devices;
                    } else {
                        $scope.inputDevices = [$scope.noneFoundLoc];
                    }

                })
                    .then(Origin.voice.selectedAudioInputDevice)
                    .then(function(value) {
                        $scope.selectedInputDevice = value;
                        $scope.$digest();
                    });

                // get the output devices
                Origin.voice.audioOutputDevices().then(function(devices) {
                    if (devices.length > 0) {
                        $scope.outputDevices = devices;
                    } else {
                        $scope.outputDevices = [$scope.noneFoundLoc];
                    }
                })
                    .then(Origin.voice.selectedAudioOutputDevice)
                    .then(function(value) {
                        $scope.selectedOutputDevice = value;
                        $scope.$digest();
                    });
            }
        }

        function enableInputLevelAndThreshold(/*enable*/) {
            onVoiceLevel(0);
            onVoiceUnderThreshold();
        }

        function onVoiceDeviceAdded() {
            setupDeviceListAndSelection();
            $scope.$digest();
        }

        function onVoiceDeviceRemoved() {
            setupDeviceListAndSelection();
            $scope.$digest();
        }

        function onVoiceEnableTestMicrophone(resetLink) {
            self.updateTestMicrophone(true, resetLink);
            $scope.$digest();
        }

        function onVoiceDisableTestMicrophone(resetLink) {
            self.updateTestMicrophone(false, resetLink);
            $scope.$digest();
        }

        function onVoiceConnected() {
            enableInputLevelAndThreshold(true);
        }

        function onVoiceDisconnected() {
            enableInputLevelAndThreshold(false);
        }

        function onVoiceLevel(value) { // value is 0 - 100
            var isMuted = false,
                activeVoiceConversationId = VoiceFactory.activeVoiceConversationId();
            
            if (activeVoiceConversationId !== 0) {
                isMuted = VoiceFactory.voiceCallInfoForConversation(activeVoiceConversationId).state === 'LOCAL_MUTED';
            }
            
            if (isMuted || !$scope.testingMicrophone) {
                value = 0;
            }

            $micLevelElement = $element.find('.origin-settings-voice-audio-test');

            $micLevelBar0 = $micLevelElement.find('.b0');
            $micLevelBar1 = $micLevelElement.find('.b1');
            $micLevelBar2 = $micLevelElement.find('.b2');
            $micLevelBar3 = $micLevelElement.find('.b3');
            $micLevelBar4 = $micLevelElement.find('.b4');
            $micLevelBar5 = $micLevelElement.find('.b5');
            $micLevelBar6 = $micLevelElement.find('.b6');
            $micLevelBar7 = $micLevelElement.find('.b7');
            $micLevelBar8 = $micLevelElement.find('.b8');
            $micLevelBar9 = $micLevelElement.find('.b9');
            $micLevelBar10 = $micLevelElement.find('.b10');
            $micLevelBar11 = $micLevelElement.find('.b11');
            $micLevelBar12 = $micLevelElement.find('.b12');
            $micLevelBar13 = $micLevelElement.find('.b13');
            $micLevelBar14 = $micLevelElement.find('.b14');
            $micLevelBar15 = $micLevelElement.find('.b15');
            $micLevelBar16 = $micLevelElement.find('.b16');
            $micLevelBar17 = $micLevelElement.find('.b17');
            $micLevelBar18 = $micLevelElement.find('.b18');
            $micLevelBar19 = $micLevelElement.find('.b19');
            $micLevelBar20 = $micLevelElement.find('.b20');

            var MIC_LEVEL_INC = 100.0 / 21.0;

            $micLevelBar0.toggleClass('lit', value > MIC_LEVEL_INC * 0.25 ? true : false);
            $micLevelBar1.toggleClass('lit', value > MIC_LEVEL_INC ? true : false);
            $micLevelBar2.toggleClass('lit', value > (MIC_LEVEL_INC * 2.0) ? true : false);
            $micLevelBar3.toggleClass('lit', value > (MIC_LEVEL_INC * 3.0) ? true : false);
            $micLevelBar4.toggleClass('lit', value > (MIC_LEVEL_INC * 4.0) ? true : false);
            $micLevelBar5.toggleClass('lit', value > (MIC_LEVEL_INC * 5.0) ? true : false);
            $micLevelBar6.toggleClass('lit', value > (MIC_LEVEL_INC * 6.0) ? true : false);
            $micLevelBar7.toggleClass('lit', value > (MIC_LEVEL_INC * 7.0) ? true : false);
            $micLevelBar8.toggleClass('lit', value > (MIC_LEVEL_INC * 8.0) ? true : false);
            $micLevelBar9.toggleClass('lit', value > (MIC_LEVEL_INC * 9.0) ? true : false);
            $micLevelBar10.toggleClass('lit', value > (MIC_LEVEL_INC * 10.0) ? true : false);
            $micLevelBar11.toggleClass('lit', value > (MIC_LEVEL_INC * 11.0) ? true : false);
            $micLevelBar12.toggleClass('lit', value > (MIC_LEVEL_INC * 12.0) ? true : false);
            $micLevelBar13.toggleClass('lit', value > (MIC_LEVEL_INC * 13.0) ? true : false);
            $micLevelBar14.toggleClass('lit', value > (MIC_LEVEL_INC * 14.0) ? true : false);
            $micLevelBar15.toggleClass('lit', value > (MIC_LEVEL_INC * 15.0) ? true : false);
            $micLevelBar16.toggleClass('lit', value > (MIC_LEVEL_INC * 16.0) ? true : false);
            $micLevelBar17.toggleClass('lit', value > (MIC_LEVEL_INC * 17.0) ? true : false);
            $micLevelBar18.toggleClass('lit', value > (MIC_LEVEL_INC * 18.0) ? true : false);
            $micLevelBar19.toggleClass('lit', value > (MIC_LEVEL_INC * 19.0) ? true : false);
            $micLevelBar20.toggleClass('lit', value > (MIC_LEVEL_INC * 20.0) ? true : false);
        }

        function onVoiceUnderThreshold() {
            $element.find('.origin-settings-sensitivity-microphone').removeClass('overThreshold');
        }

        function onVoiceOverThreshold() {
            if ($scope.testingMicrophone) {
                $element.find('.origin-settings-sensitivity-microphone').addClass('overThreshold');
            }
        }

        this.setPTTHotKeyInputFocus = function(hasFocus) {
            if(hasFocus) {
                Origin.client.settings.pushToTalkHotkeyInputHasFocus(true);
                hotKeyAssignment = "PTTHotKey";
				$scope.pTTKeyNoError = true;
            } else {
                if (window.getSelection) {
                    window.getSelection().empty();
                }
                Origin.client.settings.pushToTalkHotkeyInputHasFocus(false);
                hotKeyAssignment = "";
            }
			$scope.$digest();
        };

        function onUpdateSettings() {
            //we wrap it in a set time out here, because this callback gets triggered by event from the C++ client
            //if we do not when the app is out of focus, the promise will not resolve until we click back in focus
            setTimeout(function() {
                Origin.client.settings.readSetting("PushToTalkKeyString").then(assignSettingToScope('pushToTalkString'));
            }, 0);
        }
        
		function addClass(queryName, className)
		{
			$element.find(queryName).addClass(className);
		}

		function removeClass(queryName, className)
		{
			$element.find(queryName).removeClass(className);
		}
		
		function resetUIErrors() {
			$scope.pTTKeyNoError = true;
			/* update UI size */
			removeClass(CLS_HKERROR3, CLS_ERROR_ADJUST);
			addClass(CLS_HKERROR3, CLS_NOERROR_ADJUST);			

			$scope.$digest();
		}

        function onSettingsError() {
			/* show the error for 5 seconds, because it is quite some text */
			$timeout(resetUIErrors, 5000, false);

            if (hotKeyAssignment === "PTTHotKey") {
				$scope.pTTKeyNoError = false;
				/* update UI size */
				addClass(CLS_HKERROR3, CLS_ERROR_ADJUST);
				removeClass(CLS_HKERROR3, CLS_NOERROR_ADJUST);			
            }
            $scope.$digest();
        }
		
        function onDestroy() {
            Origin.events.off(Origin.events.VOICE_DEVICE_ADDED, onVoiceDeviceAdded);
            Origin.events.off(Origin.events.VOICE_DEVICE_REMOVED, onVoiceDeviceRemoved);
            Origin.events.off(Origin.events.VOICE_ENABLE_TEST_MICROPHONE, onVoiceEnableTestMicrophone);
            Origin.events.off(Origin.events.VOICE_DISABLE_TEST_MICROPHONE, onVoiceDisableTestMicrophone);
            Origin.events.off(Origin.events.VOICE_CONNECTED, onVoiceConnected);
            Origin.events.off(Origin.events.VOICE_DISCONNECTED, onVoiceDisconnected);
            Origin.events.off(Origin.events.VOICE_LEVEL, onVoiceLevel);
            Origin.events.off(Origin.events.VOICE_UNDER_THRESHOLD, onVoiceUnderThreshold);
            Origin.events.off(Origin.events.VOICE_OVER_THRESHOLD, onVoiceOverThreshold);
            Origin.events.off(Origin.events.CLIENT_SETTINGS_UPDATESETTINGS, onUpdateSettings);
            Origin.events.off(Origin.events.CLIENT_SETTINGS_ERROR, onSettingsError);

            // we are exiting the Voice Settings page
            Origin.voice.setInVoiceSettings(false);
        }

        $scope.deviceSettingsLoc = localize($scope.devicesettingsstr, 'devicesettingsstr');
        $scope.selectMicLoc = localize($scope.selectmicstr, 'selectmicstr');
        $scope.selectMicHintLoc = localize($scope.selectmichintstr, 'selectmichintstr');
        $scope.micVolumeLoc = localize($scope.micvolumestr, 'micvolumestr');
        $scope.testMicLoc = localize($scope.testmicstr, 'testmicstr');
        $scope.stopTestLoc = localize($scope.stopteststr, 'stopteststr');
        $scope.selectSpkrLoc = localize($scope.selectspkrstr, 'selectspkrstr');
        $scope.selectSpkrHintLoc = localize($scope.selectspkrhintstr, 'selectspkrhintstr');
        $scope.sprkVolumeLoc = localize($scope.sprkvolumestr, 'sprkvolumestr');
        $scope.captureSettingsLoc = localize($scope.capturesettingsstr, 'capturesettingsstr');
        $scope.captureModeLoc = localize($scope.capturemodestr, 'capturemodestr');
        $scope.captureModeHintLoc = localize($scope.capturemodehintstr, 'capturemodehintstr');
        $scope.voiceActivateLoc = localize($scope.voiceactivatestr, 'voiceactivatestr');
        $scope.pushToTalkLoc = localize($scope.pushtotalkstr, 'pushtotalkstr');
        $scope.pushToTalkShortcutLoc = localize($scope.pushtotalkshortcutstr, 'pushtotalkshortcutstr');
        $scope.pushToTalkShortcutHintLoc = localize($scope.pushtotalkshortcuthintstr, 'pushtotalkshortcuthintstr');
        $scope.shortcutInUseLoc = localize($scope.shortcutinusestr, 'shortcutinusestr');
        $scope.micSensitivityLoc = localize($scope.micsensitivitystr, 'micsensitivitystr');
        $scope.micSensitivityHintLoc = localize($scope.micsensitivityhintstr, 'micsensitivityhintstr');
        $scope.voiceChatIndLoc = localize($scope.voicechatindstr, 'voicechatindstr');
        $scope.voiceChatHintLoc = localize($scope.voicechathintstr, 'voicechathintstr');
        $scope.voiceChatWarnLoc = localize($scope.voicechatwarnstr, 'voicechatwarnstr');
        $scope.voiceChatWarnHintLoc = localize($scope.voicechatwarnhintstr, 'voicechatwarnhintstr');
        $scope.defaultShortcutLoc = localize($scope.defaultshortcutstr, 'defaultshortcutstr');
        $scope.defaultPTTHotkeyLoc = localize($scope.defaultptthotkeystr, 'defaultptthotkeystr');

        $scope.testMicLinkStr = $scope.testMicLoc;

        settingToDisplayMap.PushToTalkKeyString = $scope.pushToTalkShortcutLoc;

        /* jshint camelcase: false */

        // web to client settings name map
        $scope.spa_to_client_map = {
            voice_chat_indicator: "EnableVoiceChatIndicator",
            no_warning_voice_conflict: "NoWarnAboutVoiceChatConflict",
            enable_push_to_talk: "EnablePushToTalk",
            selectedInputDevice: "VoiceInputDevice",
            selectedOutputDevice: "VoiceOutputDevice",
            microphoneVolume: "MicrophoneGain",
            speakerVolume: "SpeakerGain",
            voiceActivationThreshold: "VoiceActivationThreshold",
            pushToTalkString: "PushToTalkKeyString"
        };

        $scope.selectedInputDevice = "";
        $scope.inputDevices = [""];
        $scope.selectedOutputDevice = "";
        $scope.outputDevices = [""];
        
        $scope.voice_chat_indicator = true;
        $scope.no_warning_voice_conflict = false;
        $scope.enable_push_to_talk = "true";
        $scope.microphoneVolume = 60;
        $scope.speakerVolume = 50;
        $scope.voiceActivationThreshold = 5;
        $scope.pushToTalkString = "";

        // initialize html based on settings
        if (Origin.client.isEmbeddedBrowser()) {
            setupDeviceListAndSelection();

            // update UI from settings
            for (var p in $scope.spa_to_client_map) {
                if ($scope.spa_to_client_map.hasOwnProperty(p)) {
                    Origin.client.settings.readSetting($scope.spa_to_client_map[p]).then(assignSettingToScope(p));
                }
            }

            // connect device update signals
            Origin.events.on(Origin.events.VOICE_DEVICE_ADDED, onVoiceDeviceAdded);
            Origin.events.on(Origin.events.VOICE_DEVICE_REMOVED, onVoiceDeviceRemoved);

            // connect 'test microphone' signals
            Origin.events.on(Origin.events.VOICE_ENABLE_TEST_MICROPHONE, onVoiceEnableTestMicrophone);
            Origin.events.on(Origin.events.VOICE_DISABLE_TEST_MICROPHONE, onVoiceDisableTestMicrophone);
            Origin.events.on(Origin.events.VOICE_CONNECTED, onVoiceConnected);
            Origin.events.on(Origin.events.VOICE_DISCONNECTED, onVoiceDisconnected);

            // connect voice level signal
            Origin.events.on(Origin.events.VOICE_LEVEL, onVoiceLevel);

            // over / under threshold
            Origin.events.on(Origin.events.VOICE_UNDER_THRESHOLD, onVoiceUnderThreshold);
            Origin.events.on(Origin.events.VOICE_OVER_THRESHOLD, onVoiceOverThreshold);

            // settings
            Origin.events.on(Origin.events.CLIENT_SETTINGS_UPDATESETTINGS, onUpdateSettings);
            Origin.events.on(Origin.events.CLIENT_SETTINGS_ERROR, onSettingsError);
        }

        // when the element is destroyed make sure that we cleanup
        $scope.$on('$destroy', onDestroy);
    }

    function originSettingsVoice(ComponentsConfigFactory) {
        function originSettingsVoiceLink(scope, elem, attrs, ctrl) {
            var settingsCtrl = ctrl[0],
                voiceSettingsCtrl = ctrl[1];
            
            voiceSettingsCtrl.setElement($(elem));

            /* jshint camelcase: false */
            scope.commitSetting = function(key) {
                // write it out
                if (scope.spa_to_client_map[key]) {
                    settingsCtrl.triggerSavedToasty();

                    var setting = scope[key];

                    // special case for 'enable_push_to_talk'
                    if (key === 'enable_push_to_talk') {
                        // convert to string to boolean
                        setting = (setting === 'true');
                    }

                    // special case for 'selectedInputDevice'
                    if (key === 'selectedInputDevice') {
                        Origin.voice.changeInputDevice(setting);
                    }

                    // special case for 'selectedOutputDevice'
                    if (key === 'selectedOutputDevice') {
                        Origin.voice.changeOutputDevice(setting);
                    }

                    Origin.client.settings.writeSetting(scope.spa_to_client_map[key], setting);
                }
            };

            scope.toggleSetting = function(key) {
                scope[key] = !scope[key];
                if (scope.spa_to_client_map[key]) {
                    settingsCtrl.triggerSavedToasty();

                    var setting = scope[key];

                    // special case for 'voice_chat_indicator'
                    if (key === 'voice_chat_indicator') {
                        // These numbers correlate to an enum OriginToastManager::ToastSetting::AlertMethod
                        // 0 == TOAST_DISABLED
                        // 2 == TOAST_ALERT_ONLY
                        setting = setting ? 2 : 0;
                    }

                    // special case for 'no_warning_voice_conflict'
                    if (key === 'no_warning_voice_conflict') {
                        setting = (setting === false);
                    }

                    Origin.client.settings.writeSetting(scope.spa_to_client_map[key], setting);
                }
            };

            scope.$testMicrophoneLink = $(elem).find('.origin-settings-voice-test-microphone');


            function onPTTFocusIn() {
                voiceSettingsCtrl.setPTTHotKeyInputFocus(true);
            }

            function onPTTFocusOut() {
                voiceSettingsCtrl.setPTTHotKeyInputFocus(false);
            }

            function onPTTMouseDown(event) {
                Origin.client.settings.handlePushToTalkMouseInput(event.button);
            }

            function onTestMicClicked(event) {
                event = event;

                if (scope.$testMicrophoneLink.hasClass('otka-disabled') === false) {
                    // prevent spam
                    voiceSettingsCtrl.updateTestMicrophone(false, false);

                    if (!scope.testingMicrophone) {
                        Origin.voice.testMicrophoneStart();
                    } else {
                        // TODO: clear level indicator
                        Origin.voice.testMicrophoneStop();
                    }

                    scope.testingMicrophone = !scope.testingMicrophone;
                    scope.testMicLinkStr = scope.testingMicrophone ? scope.stopTestLoc : scope.testMicLoc;

                    scope.$digest();
                }
            }

            // Clicked 'Test Microphone' 
            angular.element(elem).on('click', '.origin-settings-voice-test-microphone', onTestMicClicked);
            angular.element(elem).on('focusin', '.voice-pttKey', onPTTFocusIn);
            angular.element(elem).on('focusout', '.voice-pttKey', onPTTFocusOut);
            angular.element(elem).on('mousedown', '.voice-pttKey', onPTTMouseDown);

            function onDestroy() {
                angular.element(elem).off('click', '.origin-settings-voice-test-microphone', onTestMicClicked);
                angular.element(elem).off('focusin', '.voice-pttKey', onPTTFocusIn);
                angular.element(elem).off('focusout', '.voice-pttKey', onPTTFocusOut);
                angular.element(elem).off('mousedown', '.voice-pttKey', onPTTMouseDown);
            }

            scope.$on('$destroy', onDestroy);
        }
        return {
            restrict: 'E',
            scope: {
                devicesettingsstr: '@devicesettingsstr',
                selectmicstr: '@selectmicstr',
                selectmichintstr: '@selectmichintstr',
                micvolumestr: '@micvolumestr',
                testmicstr: '@testmicstr',
                stopteststr: '@stopteststr',
                selectspkrstr: '@selectspkrstr',
                selectspkrhintstr: '@selectspkrhintstr',
                sprkvolumestr: '@sprkvolumestr',
                capturesettingsstr: '@capturesettingsstr',
                capturemodestr: '@capturemodestr',
                capturemodehintstr: '@capturemodehintstr',
                voiceactivatestr: '@voiceactivatestr',
                pushtotalkstr: '@pushtotalkstr',
                pushtotalkshortcutstr: '@pushtotalkshortcutstr',
                pushtotalkshortcuthintstr: '@pushtotalkshortcuthintstr',
                shortcutinusestr: '@shortcutinusestr',
                micsensitivitystr: '@micsensitivitystr',
                micsensitivityhintstr: '@micsensitivityhintstr',
                voicechatindstr: '@voicechatindstr',
                voicechathintstr: '@voicechathintstr',
                voicechatwarnstr: '@voicechatwarnstr',
                voicechatwarnhintstr: '@voicechatwarnhintstr',
                nonefoundstr: '@nonefoundstr',
                mousebuttonstr: '@mousebuttonstr',
                defaultshortcutstr: '@defaultshortcutstr',
                defaultptthotkeystr: '@defaultptthotkeystr'
            },
            require: ['^originSettings', 'originSettingsVoice'],
            controller: 'OriginSettingsVoiceCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('settings/views/voice.html'),
            link: originSettingsVoiceLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSettingsVoice
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} devicesettingsstr "Device Settings"
     * @param {LocalizedString} selectmicstr "Select which microphone Origin should use."
     * @param {LocalizedString} selectmichintstr "Select which microphone Origin should use."
     * @param {LocalizedString} micvolumestr "Microphone Volume"
     * @param {LocalizedString} testmicstr "Test Microphone"
     * @param {LocalizedString} stopteststr "Stop Testing"
     * @param {LocalizedString} selectspkrstr "Select which speakers Origin should use."
     * @param {LocalizedString} selectspkrhintstr "Select which speakers Origin should use."
     * @param {LocalizedString} sprkvolumestr "Speaker Volume"
     * @param {LocalizedString} capturesettingsstr "Capture Settings"
     * @param {LocalizedString} capturemodestr "Capture Mode"
     * @param {LocalizedString} capturemodehintstr "Choose how you would like to activate voice capture."
     * @param {LocalizedString} voiceactivatestr "Voice Activation"
     * @param {LocalizedString} pushtotalkstr "Push-To-Talk"
     * @param {LocalizedString} pushtotalkshortcutstr "Push-to-talk Keyboard Shortcut"
     * @param {LocalizedString} pushtotalkshortcuthintstr "Select which key starts voice capture."
     * @param {LocalizedString} shortcutinusestr "The keyboard shortcut you tried to set is already in use or not allowed. Please select a different shortcut."
     * @param {LocalizedString} micsensitivitystr "Microphone Sensitivity"
     * @param {LocalizedString} micsensitivityhintstr "This helps filter out background noise. Set the volume level at which voice activation starts transmitting sound."
     * @param {LocalizedString} voicechatindstr "Voice Chat Indicator"
     * @param {LocalizedString} voicechathintstr "Display the username of the person talking while in game."
     * @param {LocalizedString} voicechatwarnstr "Warn me when starting a new voice chat"
     * @param {LocalizedString} voicechatwarnhintstr "Enable this to get a warning that starting a new voice chat will end your current session."
     * @param {LocalizedString} nonefoundstr "None Found"
     * @param {LocalizedString} mousebuttonstr "Mouse Button %number%"
     * @param {LocalizedString} defaultshortcutstr "Default Shortcut"
     * @param {LocalizedString} defaultptthotkeystr "Ctrl"
     * @description
     *
     * Settings section 
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-settings-voice
     *             deviceSettingsStr="Device Settings"
     *             selectmicstr="Select which microphone Origin should use."
     *             selectmichintstr="Select which microphone Origin should use."
     *             micvolumestr="Microphone Volume"
     *             testmicstr="Test Microphone"
     *             stopteststr="Stop Testing"
     *             selectspkrstr="Select which speakers Origin should use."
     *             selectspkrhintstr="Select which speakers Origin should use."
     *             sprkvolumestr="Speaker Volume"
     *             capturesettingsstr="Capture Settings"
     *             capturemodestr="Capture Mode"
     *             capturemodehintstr="Choose how you would like to activate voice capture."
     *             voiceactivatestr="Voice Activation"
     *             pushtotalkstr="Push-To-Talk"
     *             pushtotalkshortcutstr="Push-to-talk Keyboard Shortcut"
     *             pushtotalkshortcuthintstr="Select which key starts voice capture."
     *             shortcutinusestr="The keyboard shortcut you tried to set is already in use or not allowed. Please select a different shortcut."
     *             micsensitivitystr="Microphone Sensitivity"
     *             micsensitivityhintstr="This helps filter out background noise. Set the volume level at which voice activation starts transmitting sound."
     *             voicechatindstr="Voice Chat Indicator"
     *             voicechathintstr="Display the username of the person talking while in game."
     *             voicechatwarnstr="Warn me when starting a new voice chat"
     *             voicechatwarnhintstr="Enable this to get a warning that starting a new voice chat will end your current session."
     *             nonefoundstr="None Found"
     *             mousebuttonstr="Mouse Button %number%"
     *             defaultshortcutstr="Default Shortcut"
     *             defaultptthotkeystr="Ctrl"
     *         ></origin-settings-voice>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginSettingsVoiceCtrl', OriginSettingsVoiceCtrl)
        .directive('originSettingsVoice', originSettingsVoice);
}());