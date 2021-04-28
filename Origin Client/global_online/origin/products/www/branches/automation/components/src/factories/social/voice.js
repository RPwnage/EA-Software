(function() {
    'use strict';

    function VoiceFactory($interval, UtilFactory, ObservableFactory) {

        var myEvents = new Origin.utils.Communicator(),
            voiceCallStates = [],
            voiceCallObservable = ObservableFactory.observe({isInVoice: false, isCalling: false, isLocalMuted: false, hasIncomingCall: false}),
            voiceCallDurationObservable = ObservableFactory.observe({seconds: 0}),
            callDurationTimer,
            activeVoiceConversationId = 0,
            microphoneWasDisconnected = false,
            audioInputDevice = '';

        function makeVoiceCallState(nucleusId, event, originId) {
            return {
                nucleusId: nucleusId,
                state: event,
                originId: originId
            };
        }
        
        function handleVoiceCallEvent(voiceCallEventObj) {
            var event = voiceCallEventObj.event,
                nucleusId = Number(voiceCallEventObj.nucleusId),
                where = voiceCallStates.findIndex( function(element) {
                    return element.nucleusId === nucleusId;
                }),
                currentState;
                
            if (where === -1) {
                where = voiceCallStates.push(makeVoiceCallState(nucleusId, '', '')) - 1;
            }
            currentState = voiceCallStates[where].state;
            
            // ignore incoming calls if we have an outgoing call in progress
            if (currentState === 'OUTGOING' && event === 'INCOMING') {
                return;
            }
            
            switch (event) {
                case 'INCOMING':
                    Origin.voice.playIncomingRing();
                    voiceCallObservable.data.hasIncomingCall = true;
                    break;

                case 'OUTGOING':
                    voiceCallObservable.data.isCalling = true;
                    Origin.voice.playOutgoingRing();
                    break;

                case 'STARTED':
                    Origin.voice.stopIncomingRing();
                    Origin.voice.stopOutgoingRing();
                    startCall(nucleusId);
                    break;

                case 'ENDED':
                case 'NOANSWER':
                    Origin.voice.stopOutgoingRing();
                    voiceCallObservable.data.isCalling = false;
                    endCall();
                    break;

                case 'MISSED':
                case 'ENDED_INCOMING':
                    Origin.voice.stopIncomingRing();
                    resetCall();
                    break;
                    
                case 'INACTIVITY':
                case 'ENDED_UNEXPECTEDLY':
                case 'CONNECT_ERROR':
                    resetCall();
                    break;
            }
            voiceCallStates[where].state = voiceCallEventObj.event;
            myEvents.fire('voiceCallEvent', voiceCallEventObj);
        }

        function handleVoiceLevel(level) {
            myEvents.fire('voiceLevel', level);
        }

        function handleVoiceDeviceAdded(/*deviceName*/) {
            if (Origin.voice.supported()) {
                Origin.voice.selectedAudioInputDevice().then(function(inputDevice) {
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
                });
            }
        }

        function handleVoiceDeviceRemoved() {
            if (Origin.voice.supported()) {
                Origin.voice.audioInputDevices().then(function(devices) {
                    if (devices.length === 0) {
                        // If there are no devices connected now then obviously the microphone was unplugged
                        microphoneWasDisconnected = true;
                        myEvents.fire('voiceInputDeviceChanged');
                    }
                    else {
                        // Otherwise see if the device that was removed is the one we were previously using
                        Origin.voice.selectedAudioInputDevice().then(function(inputDevice) {
                            if (inputDevice !== audioInputDevice) {
                                // The input device has changed as a result of unplugging the current device
                                audioInputDevice = inputDevice;
                                myEvents.fire('voiceInputDeviceChanged', inputDevice);
                            }
                        });
                    }
                });
            }
        }
        
        function startCall(conversationId) {
            activeVoiceConversationId = conversationId;
            voiceCallObservable.data.isInVoice = true;
            voiceCallObservable.data.isCalling = false;
            voiceCallObservable.data.hasIncomingCall = false;
            voiceCallObservable.commit();
            startCallTimer();
        }

        function endCall() {
            endCallTimer();
            voiceCallObservable.data.isInVoice = false;
            voiceCallObservable.commit();
            activeVoiceConversationId = 0;
        }

        function resetCall() {
            resetCallTimer();
            voiceCallObservable.data.isInVoice = false;
            voiceCallObservable.data.isCalling = false;
            voiceCallObservable.data.hasIncomingCall = false;
            voiceCallObservable.commit();
            activeVoiceConversationId = 0;
        }

        function startCallTimer() {
            if (!angular.isDefined(callDurationTimer)) {
                voiceCallDurationObservable.data.seconds = 0;

                callDurationTimer = $interval(function () {
                    // call callDurationTimer
                    ++voiceCallDurationObservable.data.seconds;
                    voiceCallDurationObservable.commit();
                }, 1000);
            }
        }

        function endCallTimer() {
            if (angular.isDefined(callDurationTimer)) {
                $interval.cancel(callDurationTimer);
                callDurationTimer = undefined;
            }
            
            if (voiceCallDurationObservable.data.seconds) {
                voiceCallDurationObservable.data.seconds = 0;
                voiceCallDurationObservable.commit();
            }
        }

        function resetCallTimer() {
            endCallTimer();
        }

        function init() {
            Origin.events.on(Origin.events.VOICE_CALL, handleVoiceCallEvent);
            Origin.events.on(Origin.events.VOICE_LEVEL, handleVoiceLevel);

            if (Origin.voice.supported()) {
                // We need to watch for when the microphone is disconected/connected
                Origin.voice.audioInputDevices().then(function(devices) {
                    Origin.voice.selectedAudioInputDevice().then(function(inputDevice) {
                        audioInputDevice = inputDevice;
                        if (devices.length === 0) {
                            microphoneWasDisconnected = true;
                        }
                        Origin.events.on(Origin.events.VOICE_DEVICE_ADDED, UtilFactory.reverseThrottle( function(deviceName) {
                            handleVoiceDeviceAdded(deviceName);
                        }, 1000));
                        Origin.events.on(Origin.events.VOICE_DEVICE_REMOVED, UtilFactory.reverseThrottle( function() {
                            handleVoiceDeviceRemoved();
                        }, 1000));
                    });
                });
            }
        }

        return {

            init: init,
            events: myEvents,

            activeVoiceConversationId: function () {
                return activeVoiceConversationId;
            },

            setActiveVoiceConversationId: function (id) {
                activeVoiceConversationId = id;
            },

            incomingVoiceConversationId: function () {
                var callInfo = voiceCallStates.find( function(element) {
                    return element.state === 'INCOMING';
                });
                return _.get(callInfo, ['nucleusId'], 0);
            },

            voiceCallInfoForConversation: function (nucleusId) {
                var callInfo = voiceCallStates.find( function(element) {
                    return element.nucleusId === nucleusId;
                });
                if (angular.isUndefined(callInfo)) {
                    return { nucleusId: nucleusId, state: '', originId: ''};
                }
                return callInfo;
            },

            setVoiceCallInfoForConversation: function (nucleusId, event, originId) {
                var where = voiceCallStates.findIndex( function(element) {
                    return element.nucleusId === nucleusId;
                });
                if (where !== -1) {
                    voiceCallStates[where].state = event;
                    voiceCallStates[where].originId = originId;
                } else {
                    voiceCallStates.push(makeVoiceCallState(nucleusId, event, originId));
                }
            },

            toggleMute: function() {
                voiceCallObservable.data.isLocalMuted = !voiceCallObservable.data.isLocalMuted;
                if (voiceCallObservable.data.isLocalMuted) {
                    Origin.voice.muteSelf();
                } else {
                    Origin.voice.unmuteSelf();
                }
                return voiceCallObservable.data.isLocalMuted;
            },
            
			voiceCallObservable: function() {
				return voiceCallObservable;
			},
            
			voiceCallDurationObservable: function() {
				return voiceCallDurationObservable;
			}
			
        };
    }

     /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function VoiceFactorySingleton($interval, UtilFactory, ObservableFactory, SingletonRegistryFactory) {
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