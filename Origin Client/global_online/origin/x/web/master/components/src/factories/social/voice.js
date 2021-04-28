(function() {
    'use strict';

    function VoiceFactory(UtilFactory) {

        var myEvents = new Origin.utils.Communicator(),
            voiceCallState,
            activeVoiceConversationId = 0,
            incomingVoiceCallInfo = {
                nucleusId: 0,
                originId: ''
            },
            outgoingVoiceCallInfo = {
                nucleusId: 0,
                originId: ''
            },
            microphoneWasDisconnected = false,
            audioInputDevice = '';

        function handleVoiceCallEvent(voiceCallEventObj) {
            voiceCallState = voiceCallEventObj.event;
            var event = voiceCallEventObj.event;
            switch (event) {
                case 'INCOMING':
                    Origin.voice.playIncomingRing();
                    break;

                case 'OUTGOING':
                    Origin.voice.playOutgoingRing();
                    break;

                case 'STARTED':
                    Origin.voice.stopIncomingRing();
                    Origin.voice.stopOutgoingRing();
                    break;

                case 'ENDED':
                case 'NOANSWER':
                    Origin.voice.stopOutgoingRing();
                    break;

                case 'MISSED':
                case 'ENDED_INCOMING':
                    Origin.voice.stopIncomingRing();
                    break;
            }
            myEvents.fire('voiceCallEvent', voiceCallEventObj);
        }

        function handleVoiceLevel(level) {
            myEvents.fire('voiceLevel', level);
        }

        function handleVoiceDeviceAdded(/*deviceName*/) {
            if (Origin.voice.supported()) {
                var inputDevice = Origin.voice.selectedAudioInputDevice();
                if (microphoneWasDisconnected) {
                    // Previously there was no microphone, now there is
                    microphoneWasDisconnected = false;
                    myEvents.fire('voiceInputDeviceChanged', inputDevice);
                }
                else if (inputDevice !== audioInputDevice) {
                    // The input device has changed as a result of plugging in a new device
                    audioInputDevice = inputDevice;
                    myEvents.fire('voiceInputDeviceChanged', inputDevice);
                }
            }
        }

        function handleVoiceDeviceRemoved() {
            if (Origin.voice.supported()) {
                var devices = Origin.voice.audioInputDevices();
                if (devices.length === 0) {
                    // If there are no devices connected now then obviously the microphone was unplugged
                    microphoneWasDisconnected = true;
                    myEvents.fire('voiceInputDeviceChanged');
                }
                else {
                    // Otherwise see if the device that was removed is the one we were previously using
                    var inputDevice = Origin.voice.selectedAudioInputDevice();
                    if (inputDevice !== audioInputDevice) {
                        // The input device has changed as a result of unplugging the current device
                        audioInputDevice = inputDevice;
                        myEvents.fire('voiceInputDeviceChanged', inputDevice);
                    }
                }
            }
        }

        function init() {
            Origin.events.on(Origin.events.VOICE_CALL, handleVoiceCallEvent);
            Origin.events.on(Origin.events.VOICE_LEVEL, handleVoiceLevel);

            if (Origin.voice.supported()) {
                // We need to watch for when the microphone is disconected/connected
                var devices = Origin.voice.audioInputDevices();
                audioInputDevice = Origin.voice.selectedAudioInputDevice();
                if (devices.length === 0) {
                    microphoneWasDisconnected = true;
                }
                Origin.events.on(Origin.events.VOICE_DEVICE_ADDED, UtilFactory.reverseThrottle( function(deviceName) {
                    handleVoiceDeviceAdded(deviceName);
                }, 1000));
                Origin.events.on(Origin.events.VOICE_DEVICE_REMOVED, UtilFactory.reverseThrottle( function() {
                    handleVoiceDeviceRemoved();
                }, 1000));
            }
        }

        return {

            init: init,
            events: myEvents,

            voiceCallState: function () {
                return voiceCallState;
            },

            getActiveVoiceConversationId: function () {
                return activeVoiceConversationId;
            },

            setActiveVoiceConversationId: function (id) {
                activeVoiceConversationId = id;
            },

            getIncomingVoiceCallInfo: function () {
                return incomingVoiceCallInfo;
            },

            setIncomingVoiceCallInfo: function (nucleusId, originId) {
                incomingVoiceCallInfo.nucleusId = nucleusId;
                incomingVoiceCallInfo.originId = originId;
            },

            getOutgoingVoiceCallInfo: function () {
                return outgoingVoiceCallInfo;
            },

            setOutgoingVoiceCallInfo: function (nucleusId, originId) {
                outgoingVoiceCallInfo.nucleusId = nucleusId;
                outgoingVoiceCallInfo.originId = originId;
            }
        };
    }

     /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function VoiceFactorySingleton(UtilFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('VoiceFactory', VoiceFactory, this, arguments);
    }
    /**
     * @ngdoc service
     * @name origin-components.factories.VoiceFactory
     
     * @description
     *
     * VoiceFactory
     */
    angular.module('origin-components')
        .factory('VoiceFactory', VoiceFactorySingleton);
}());