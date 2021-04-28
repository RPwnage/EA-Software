/**
 * @file voice.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-settings-voice';

    function OriginSettingsVoiceCtrl($scope, $timeout, UtilFactory, VoiceFactory) {
        var self = this;
        var $element, $micLevelElement,
            $micLevelBar0, $micLevelBar1, $micLevelBar2, $micLevelBar3, $micLevelBar4,
            $micLevelBar5, $micLevelBar6, $micLevelBar7, $micLevelBar8, $micLevelBar9,
            $micLevelBar10, $micLevelBar11, $micLevelBar12, $micLevelBar13, $micLevelBar14,
            $micLevelBar15, $micLevelBar16, $micLevelBar17, $micLevelBar18, $micLevelBar19,
            $micLevelBar20;

        $scope.testingMicrophone = false;


        var throttledDigestFunction = UtilFactory.throttle(function() {
            $timeout(function() {
                $scope.$digest();
            }, 0, false);
        }, 1000);

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

                // special case for 'selectedInputDevice'
                if (prop === 'selectedInputDevice') {
                    if ($scope.selectedInputDevices.indexOf(setting) === -1) {
                        setting = $scope.selectedInputDevice;
                    }
                }

                // special case for 'selectedOutputDevice'
                if (prop === 'selectedOutputDevice') {
                    if ($scope.selectedOutputDevices.indexOf(setting) === -1) {
                        setting = $scope.selectedOutputDevice;
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
            }
            else
            {
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

        $scope.noneFoundLoc = UtilFactory.getLocalizedStr($scope.noneFoundStr, CONTEXT_NAMESPACE, 'noneFoundStr');

        function setupDeviceListAndSelection() {
            var devices;

            if (Origin.voice.supported()) {
                // get the input devices
                devices = Origin.client.voice.audioInputDevices();
                if (devices.length > 0) {
                    $scope.inputDevices = devices;
                }
                else {
                    $scope.inputDevices = [$scope.noneFoundLoc];
                }

                // get the output devices
                devices = Origin.client.voice.audioOutputDevices();
                if (devices.length > 0) {
                    $scope.outputDevices = devices;
                }
                else {
                    $scope.outputDevices = [$scope.noneFoundLoc];
                }

                $scope.selectedInputDevice = Origin.client.voice.selectedAudioInputDevice();
                $scope.selectedOutputDevice = Origin.client.voice.selectedAudioOutputDevice();
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
            if (VoiceFactory.voiceCallState() === 'LOCAL_MUTED' || !$scope.testingMicrophone) {
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

            // we are exiting the Voice Settings page
            Origin.client.voice.setInVoiceSettings(false);
        }

        $scope.deviceSettingsStr = UtilFactory.getLocalizedStr($scope.deviceSettingsStr, CONTEXT_NAMESPACE, 'deviceSettingsStr');
        $scope.selectMicStr = UtilFactory.getLocalizedStr($scope.selectMicStr, CONTEXT_NAMESPACE, 'selectMicStr');
        $scope.selectMicHintStr = UtilFactory.getLocalizedStr($scope.selectMicHintStr, CONTEXT_NAMESPACE, 'selectMicHintStr');
        $scope.micVolumeStr = UtilFactory.getLocalizedStr($scope.micVolumeStr, CONTEXT_NAMESPACE, 'micVolumeStr');
        $scope.testMicLoc = UtilFactory.getLocalizedStr($scope.testMicStr, CONTEXT_NAMESPACE, 'testMicStr');
        $scope.stopTestLoc = UtilFactory.getLocalizedStr($scope.stopTestStr, CONTEXT_NAMESPACE, 'stopTestStr');
        $scope.selectSpkrStr = UtilFactory.getLocalizedStr($scope.selectSpkrStr, CONTEXT_NAMESPACE, 'selectSpkrStr');
        $scope.selectSpkrHintStr = UtilFactory.getLocalizedStr($scope.selectSpkrHintStr, CONTEXT_NAMESPACE, 'selectSpkrHintStr');
        $scope.sprkVolumeStr = UtilFactory.getLocalizedStr($scope.sprkVolumeStr, CONTEXT_NAMESPACE, 'sprkVolumeStr');
        $scope.captureSettingsStr = UtilFactory.getLocalizedStr($scope.captureSettingsStr, CONTEXT_NAMESPACE, 'captureSettingsStr');
        $scope.captureModeStr = UtilFactory.getLocalizedStr($scope.captureModeStr, CONTEXT_NAMESPACE, 'captureModeStr');
        $scope.captureModeHintStr = UtilFactory.getLocalizedStr($scope.captureModeHintStr, CONTEXT_NAMESPACE, 'captureModeHintStr');
        $scope.voiceActivateStr = UtilFactory.getLocalizedStr($scope.voiceActivateStr, CONTEXT_NAMESPACE, 'voiceActivateStr');
        $scope.pushToTalkStr = UtilFactory.getLocalizedStr($scope.pushToTalkStr, CONTEXT_NAMESPACE, 'pushToTalkStr');
        $scope.pushToTalkShortcutStr = UtilFactory.getLocalizedStr($scope.pushToTalkShortcutStr, CONTEXT_NAMESPACE, 'pushToTalkShortcutStr');
        $scope.pushToTalkShortcutHintStr = UtilFactory.getLocalizedStr($scope.pushToTalkShortcutHintStr, CONTEXT_NAMESPACE, 'pushToTalkShortcutHintStr');
        $scope.restoreDefaultStr = UtilFactory.getLocalizedStr($scope.restoreDefaultStr, CONTEXT_NAMESPACE, 'restoreDefaultStr');
        $scope.shortcutInUseStr = UtilFactory.getLocalizedStr($scope.shortcutInUseStr, CONTEXT_NAMESPACE, 'shortcutInUseStr');
        $scope.micSensitivityStr = UtilFactory.getLocalizedStr($scope.micSensitivityStr, CONTEXT_NAMESPACE, 'micSensitivityStr');
        $scope.micSensitivityHintStr = UtilFactory.getLocalizedStr($scope.micSensitivityHintStr, CONTEXT_NAMESPACE, 'micSensitivityHintStr');
        $scope.voiceChatIndStr = UtilFactory.getLocalizedStr($scope.voiceChatIndStr, CONTEXT_NAMESPACE, 'voiceChatIndStr');
        $scope.voiceChatHintStr = UtilFactory.getLocalizedStr($scope.voiceChatHintStr, CONTEXT_NAMESPACE, 'voiceChatHintStr');
        $scope.voiceChatWarnStr = UtilFactory.getLocalizedStr($scope.voiceChatWarnStr, CONTEXT_NAMESPACE, 'voiceChatWarnStr');
        $scope.voiceChatWarnHintStr = UtilFactory.getLocalizedStr($scope.voiceChatWarnHintStr, CONTEXT_NAMESPACE, 'voiceChatWarnHintStr');

        $scope.testMicLinkStr = $scope.testMicLoc;

        /* jshint camelcase: false */

        // web to client settings name map
        $scope.spa_to_client_map = {
            voice_chat_indicator: "EnableVoiceChatIndicator",
            no_warning_voice_conflict: "NoWarnAboutVoiceChatConflict",
            enable_push_to_talk: "EnablePushToTalk",
            selectedInputDevice: "VoiceInputDevice",
            selectedOutputDevice: "VoiceOutputDevice"
        };

        $scope.selectedInputDevice = "";
        $scope.inputDevices = [""];
        $scope.selectedOutputDevice = "";
        $scope.outputDevices = [""];
        
        $scope.voice_chat_indicator = true;
        $scope.no_warning_voice_conflict = false;
        $scope.enable_push_to_talk = "true";

        // initialize html based on settings
        if (Origin.client.isEmbeddedBrowser()) {
            setupDeviceListAndSelection();

            // set up sliders


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
                        Origin.client.voice.changeInputDevice(setting);
                    }

                    // special case for 'selectedOutputDevice'
                    if (key === 'selectedOutputDevice') {
                        Origin.client.voice.changeOutputDevice(setting);
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

            // Clicked 'Test Microphone' 
            angular.element(elem).on('click', '.origin-settings-voice-test-microphone', function(event) {
                event = event;

                if (scope.$testMicrophoneLink.hasClass('otka-disabled') === false) {
                    // prevent spam
                    voiceSettingsCtrl.updateTestMicrophone(false, false);

                    if (!scope.testingMicrophone) {
                        Origin.client.voice.testMicrophoneStart();
                    }
                    else {
                        // TODO: clear level indicator
                        Origin.client.voice.testMicrophoneStop();
                    }

                    scope.testingMicrophone = !scope.testingMicrophone;
                    scope.testMicLinkStr = scope.testingMicrophone ? scope.stopTestLoc : scope.testMicLoc;

                    scope.$digest();
                }
            });
        }
        return {
            restrict: 'E',
            scope: {
                testMicStr: '@testMicStr',
                stopTestStr: '@stopTestStr',
                noneFoundStr: '@noneFoundStr'
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
     * @param {LocalizedString} testMicStr "Test Microphone"
     * @param {LocalizedString} stopTestStr "Stop Testing"
     * @param {LocalizedString} noneFoundStr "None Found"
     * @description
     *
     * Settings section 
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-settings-voice
     *             nonefoundstr="None Found"
     *         ></origin-settings-voice>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginSettingsVoiceCtrl', OriginSettingsVoiceCtrl)
        .directive('originSettingsVoice', originSettingsVoice);
}());