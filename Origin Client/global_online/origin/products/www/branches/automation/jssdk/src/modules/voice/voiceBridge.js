/*jshint strict: false */
/*jshint unused: false */
/*jshint undef: false */
define([
    'core/events',
    'modules/client/client',
    'modules/client/communication'
], function(events, client, communication) {

    function handleVoiceCallEvent(voiceCallEventObj) {
        if (client.voice.supported()) {
            events.fire(events.VOICE_CALL, voiceCallEventObj);
        }
    }

    function handleVoiceLevel(level) {
        if (client.voice.supported()) {
            events.fire(events.VOICE_LEVEL, level);
        }
    }

    function handleDeviceAdded(deviceName) {
        if (client.voice.supported()) {
            events.fire(events.VOICE_DEVICE_ADDED, deviceName);
        }
    }

    function handleDeviceRemoved() {
        if (client.voice.supported()) {
            events.fire(events.VOICE_DEVICE_REMOVED);
        }
    }

    function handleDefaultDeviceChanged() {
        if (client.voice.supported()) {
            events.fire(events.VOICE_DEFAULT_DEVICE_CHANGED);
        }
    }

    // WinXP
    function handleDeviceChanged() {
        if (client.voice.supported()) {
            events.fire(events.VOICE_DEVICE_CHANGED);
        }
    }

    function handleUnderThreshold() {
        if (client.voice.supported()) {
            events.fire(events.VOICE_UNDER_THRESHOLD);
        }
    }

    function handleOverThreshold() {
        if (client.voice.supported()) {
            events.fire(events.VOICE_OVER_THRESHOLD);
        }
    }

    function handleVoiceConnected() {
        if (client.voice.supported()) {
            events.fire(events.VOICE_CONNECTED);
        }
    }

    function handleVoiceDisconnected() {
        if (client.voice.supported()) {
            events.fire(events.VOICE_DISCONNECTED);
        }
    }

    function handleEnableTestMicrophone(resetLink) {
        if (client.voice.supported()) {
            events.fire(events.VOICE_ENABLE_TEST_MICROPHONE, resetLink);
        }
    }

    function handleDisableTestMicrophone(resetLink) {
        if (client.voice.supported()) {
            events.fire(events.VOICE_DISABLE_TEST_MICROPHONE, resetLink);
        }
    }

    function handleClearLevelIndicator() {
        if (client.voice.supported()) {
            events.fire(events.VOICE_CLEAR_LEVEL_INDICATOR);
        }
    }

    function init() {
        //listen for voice events
        events.on(events.CLIENT_VOICE_VOICECALLEVENT, handleVoiceCallEvent);
        events.on(events.CLIENT_VOICE_VOICELEVEL, handleVoiceLevel);
        events.on(events.CLIENT_VOICE_DEVICEADDED, handleDeviceAdded);
        events.on(events.CLIENT_VOICE_DEVICEREMOVED, handleDeviceRemoved);
        events.on(events.CLIENT_VOICE_DEFAULTDEVICECHANGED, handleDefaultDeviceChanged);
        events.on(events.CLIENT_VOICE_DEVICECHANGED, handleDeviceChanged);
        events.on(events.CLIENT_VOICE_UNDERTHRESHOLD, handleUnderThreshold);
        events.on(events.CLIENT_VOICE_OVERTHRESHOLD, handleOverThreshold);
        events.on(events.CLIENT_VOICE_VOICECONNECTED, handleVoiceConnected);
        events.on(events.CLIENT_VOICE_VOICEDISCONNECTED, handleVoiceDisconnected);
        events.on(events.CLIENT_VOICE_ENABLETESTMICROPHONE, handleEnableTestMicrophone);
        events.on(events.CLIENT_VOICE_DISABLETESTMICROPHONE, handleDisableTestMicrophone);
        events.on(events.CLIENT_VOICE_CLEARLEVELINDICATOR, handleClearLevelIndicator);
    }

    /**
     * Voice module
     * @module module:voice
     * @memberof module:Origin
     */
    return /** @lends module:Origin.module:voice*/ {
        init: init,

        /**
         * is voice supported
         * @method
         * @returns {boolean}
         */
        supported: function() {
            return client.voice.supported();
        },

        /**
         * is voice supported by the friend identified by their nucleus id
         * @module module:voice
         * @memberof module:Origin
         * @returns boolean
         */
        isSupportedBy: function(friendNucleusId) {
            return client.voice.isSupportedBy(friendNucleusId);
        },

        /**
         * set wheterh user is on the voice settings page
         * @method
         * @param {bool} indicates whether user is on the voice settings page
         * @returns {void} 
         */
        setInVoiceSettings: function(inVoiceSettings) {
            if (client.voice.supported()) {
                client.voice.setInVoiceSettings(inVoiceSettings);
            }
        },

        /**
         * join a voice call
         * @method
         * @param {array} list of participants
         * @returns {void}
         */
        joinVoice: function(id, participants) {
            if (client.voice.supported()) {
                client.voice.joinVoice(id, participants);
            }
        },

        /**
         * leave the voice call
         * @method
         * @returns {void}
         */
        leaveVoice: function(id) {
            if (client.voice.supported()) {
                client.voice.leaveVoice(id);
            }
        },

        /**
         * answer a voice call
         * @method
         * @returns {void}
         */
        answerCall: function(id) {
            if (client.voice.supported()) {
                client.voice.joinVoice(id, []);
            }
        },

        /**
         * ignore a voice call
         * @method
         * @returns {void}
         */
        ignoreCall: function(id) {
            if (client.voice.supported()) {
                client.voice.leaveVoice(id);
            }
        },

        /**
         * start testing microphone
         * @method
         * @returns {void}
         */
        testMicrophoneStart: function() {
            if (client.voice.supported()) {
                client.voice.testMicrophoneStart();
            }
        },

        /**
         * stop testing microphone
         * @method
         * @returns {void}
         */
        testMicrophoneStop: function() {
            if (client.voice.supported()) {
                client.voice.testMicrophoneStop();
            }
        },

        /**
         * change input device
         * @method
         * @param {string} name of device
         * @returns {void}
         */
        changeInputDevice: function(device) {
            if (client.voice.supported()) {
                client.voice.changeInputDevice(device);
            }
        },

        /**
         * change output device
         * @method
         * @param {string} name of device
         * @returns {void}
         */
        changeOutputDevice: function(device) {
            if (client.voice.supported()) {
                client.voice.changeOutputDevice(device);
            }
        },

        /**
         * play incoming ring
         * @method
         * @returns {void}
         */
        playIncomingRing: function() {
            if (client.voice.supported()) {
                client.voice.playIncomingRing();
            }
        },

        /**
         * stop incoming ring
         * @method
         * @returns {void}
         */
        stopIncomingRing: function() {
            if (client.voice.supported()) {
                client.voice.stopIncomingRing();
            }
        },

        /**
         * play outgoing ring
         * @method
         * @returns {void}
         */
        playOutgoingRing: function() {
            if (client.voice.supported()) {
                client.voice.playOutgoingRing();
            }
        },

        /**
         * stop outgoing ring
         * @method
         * @returns {void}
         */
        stopOutgoingRing: function() {
            if (client.voice.supported()) {
                client.voice.stopOutgoingRing();
            }
        },

        /**
         * mute self
         * @method
         * @returns {void}
         */
        muteSelf: function() {
            if (client.voice.supported()) {
                client.voice.muteSelf();
            }
        },

        /**
         * unmute self
         * @method
         * @returns {void}
         */
        unmuteSelf: function() {
            if (client.voice.supported()) {
                client.voice.unmuteSelf();
            }
        },

        /**
         * show toasty for voice call
         * @method
         * @returns {void}
         */
        showToast: function(event, originId, conversationId) {
            if (client.voice.supported()) {
                client.voice.showToast(event, originId, conversationId);
            }
        },

        /**
         * get list of audio input devices
         * @method
         * @returns {Array}
         */
        audioInputDevices: function() {
            if (client.voice.supported()) {
                return client.voice.audioInputDevices();
            }
        },

        /**
         * get list of audio output devices
         * @method
         * @returns {Array}
         */
        audioOutputDevices: function() {
            if (client.voice.supported()) {
                return client.voice.audioOutputDevices();
            }
        },

        /**
         * get selected audio input device
         * @method
         * @returns {string}
         */
        selectedAudioInputDevice: function() {
            if (client.voice.supported()) {
                return client.voice.selectedAudioInputDevice();
            }
        },

        /**
         * get selected audio output device
         * @method
         * @returns {string}
         */
        selectedAudioOutputDevice: function() {
            if (client.voice.supported()) {
                return client.voice.selectedAudioOutputDevice();
            }
        },

        /**
         * get network quality (0-3)
         * @method
         * @returns {int}
         */
        networkQuality: function () {
            if (client.voice.supported) {
                return client.voice.networkQuality();
            }
        },

        /**
         * get channel id of current voice chat
         * @method
         * @returns {string}
         */
        channelId: function () {
            if (client.voice.supported) {
                return client.voice.channelId();
            }
        },
        /**
         * show voice survey
         * @method
         * @returns {void}
         */
        showSurvey: function (channelId) {
            if (client.voice.supported) {
                return client.voice.showSurvey(channelId);
            }
        }
    };
});