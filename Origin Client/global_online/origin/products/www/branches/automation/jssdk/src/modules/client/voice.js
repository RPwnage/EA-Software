/*jshint unused: false */
/*jshint undef:false */
/*jshint strict: false */

/**
 * this module handles all the communication related to voice with the C++ client
 */


define([
    'modules/client/clientobjectregistry'
], function(clientobjectregistry) {

    /**
     * Contains client voice communication methods
     * @module module:voice
     * @memberof module:Origin.module:client
     *
     */

    var clientVoiceWrapper = null,
        clientObjectName = 'OriginVoice';

    function executeWhenClientObjectInitialized(clientObjectWrapper) {
        clientVoiceWrapper = clientObjectWrapper;
        if (clientVoiceWrapper.clientObject && clientVoiceWrapper.propertyFromOriginClient('supported')) {
            clientVoiceWrapper.connectClientSignalToJSSDKEvent('deviceAdded', 'CLIENT_VOICE_DEVICEADDED');
            clientVoiceWrapper.connectClientSignalToJSSDKEvent('deviceRemoved', 'CLIENT_VOICE_DEVICEREMOVED');
            clientVoiceWrapper.connectClientSignalToJSSDKEvent('defaultDeviceChanged', 'CLIENT_VOICE_DEFAULTDEVICECHANGED');
            clientVoiceWrapper.connectClientSignalToJSSDKEvent('deviceChanged', 'CLIENT_VOICE_DEVICECHANGED');
            clientVoiceWrapper.connectClientSignalToJSSDKEvent('voiceLevel', 'CLIENT_VOICE_VOICELEVEL');
            clientVoiceWrapper.connectClientSignalToJSSDKEvent('underThreshold', 'CLIENT_VOICE_UNDERTHRESHOLD');
            clientVoiceWrapper.connectClientSignalToJSSDKEvent('overThreshold', 'CLIENT_VOICE_OVERTHRESHOLD');
            clientVoiceWrapper.connectClientSignalToJSSDKEvent('voiceConnected', 'CLIENT_VOICE_VOICECONNECTED');
            clientVoiceWrapper.connectClientSignalToJSSDKEvent('voiceDisconnected', 'CLIENT_VOICE_VOICEDISCONNECTED');
            clientVoiceWrapper.connectClientSignalToJSSDKEvent('enableTestMicrophone', 'CLIENT_VOICE_ENABLETESTMICROPHONE');
            clientVoiceWrapper.connectClientSignalToJSSDKEvent('disableTestMicrophone', 'CLIENT_VOICE_DISABLETESTMICROPHONE');
            clientVoiceWrapper.connectClientSignalToJSSDKEvent('clearLevelIndicator', 'CLIENT_VOICE_CLEARLEVELINDICATOR');
            clientVoiceWrapper.connectClientSignalToJSSDKEvent('voiceCallEvent', 'CLIENT_VOICE_VOICECALLEVENT');
        }
    }

    clientobjectregistry.registerClientObject(clientObjectName).then(executeWhenClientObjectInitialized);

    return /** @lends module:Origin.module:client.module:voice */ {
        /**
         * [supported description]
         * @return {promise} [description]
         */
        supported: function() {
            return clientVoiceWrapper.propertyFromOriginClient('supported');
        },
        /**
         * [isSupportedBy description]
         * @param {promise} friendNucleusId nucleus id of friend to check voice support for
         * @return {promise} [description]
         */
        isSupportedBy: function(friendNucleusId) {
            if (typeof friendNucleusId === 'undefined') {
                return false;
            }
            return clientVoiceWrapper.sendToOriginClient('isSupportedBy', arguments);
        },
        /**
         * [setInVoiceSettings description]
         * @param {promise} inVoiceSettings [description]
         */
        setInVoiceSettings: function(inVoiceSettings) {
            return clientVoiceWrapper.sendToOriginClient('setInVoiceSettings', arguments);
        },
        /**
         * [startVoiceChannel description]
         * @return {promise} [description]
         */
        startVoiceChannel: function() {
            return clientVoiceWrapper.sendToOriginClient('startVoiceChannel');
        },
        /**
         * [stopVoiceChannel description]
         * @return {promise} [description]
         */
        stopVoiceChannel: function() {
            return clientVoiceWrapper.sendToOriginClient('stopVoiceChannel');
        },
        /**
         * [testMicrophoneStart description]
         * @return {promise} [description]
         */
        testMicrophoneStart: function() {
            return clientVoiceWrapper.sendToOriginClient('testMicrophoneStart');
        },
        /**
         * [testMicrophoneStop description]
         * @return {promise} [description]
         */
        testMicrophoneStop: function() {
            return clientVoiceWrapper.sendToOriginClient('testMicrophoneStop');
        },
        /**
         * [changeInputDevice description]
         * @param  {string} device [description]
         * @return {promise}        [description]
         */
        changeInputDevice: function(device) {
            return clientVoiceWrapper.sendToOriginClient('changeInputDevice', arguments);
        },
        /**
         * [changeOutputDevice description]
         * @param  {string} device [description]
         * @return {promise}        [description]
         */
        changeOutputDevice: function(device) {
            return clientVoiceWrapper.sendToOriginClient('changeOutputDevice', arguments);
        },
        /**
         * [playIncomingRing description]
         * @return {promise} [description]
         */
        playIncomingRing: function() {
            return clientVoiceWrapper.sendToOriginClient('playIncomingRing');
        },
        /**
         * [playOutgoingRing description]
         * @return {promise} [description]
         */
        playOutgoingRing: function() {
            return clientVoiceWrapper.sendToOriginClient('playOutgoingRing');
        },
        /**
         * [stopIncomingRing description]
         * @return {promise} [description]
         */
        stopIncomingRing: function() {
            return clientVoiceWrapper.sendToOriginClient('stopIncomingRing');
        },
        /**
         * [stopOutgoingRing description]
         * @return {promise} [description]
         */
        stopOutgoingRing: function() {
            return clientVoiceWrapper.sendToOriginClient('stopOutgoingRing');
        },
        /**
         * [joinVoice description]
         * @param  {string}     id           identifier for the conversation (for 1:1, it is the nucleus id of the other participant)
         * @param  {stringList} participants the list of participants in the conversation
         * @return {promise}              [description]
         */
        joinVoice: function(id, participants) {
            return clientVoiceWrapper.sendToOriginClient('joinVoice', arguments);
        },
        /**
         * [leaveVoice description]
         # @param  {string}  id identifier for the conversation (for 1:1, it is the nucleus id of the other participant)
         * @return {promise} [description]
         */
        leaveVoice: function(id) {
            return clientVoiceWrapper.sendToOriginClient('leaveVoice', arguments);
        },
        /**
         * [muteSelf description]
         * @return {promise} [description]
         */
        muteSelf: function() {
            return clientVoiceWrapper.sendToOriginClient('muteSelf');
        },
        /**
         * [unmuteSelf description]
         * @return {promise} [description]
         */
        unmuteSelf: function() {
            return clientVoiceWrapper.sendToOriginClient('unmuteSelf');
        },
        /**
         * [showToast description]
         * @param  {string} event          [description]
         * @param  {string} originId       [description]
         * @param  {string} conversationId [description]
         * @return {promise}                [description]
         */
        showToast: function(event, originId, conversationId) {
            return clientVoiceWrapper.sendToOriginClient('showToast', arguments);
        },

        /**
         * show survey description
         * @param  {number} channelId the channel id
         * @return {void}
         */
        showSurvey: function(channelId) {
            return clientVoiceWrapper.sendToOriginClient('showSurvey', arguments);
        },        
        /**
         * [audioInputDevices description]
         * @return {promise} [description]
         */
        audioInputDevices: function() {
            return clientVoiceWrapper.sendToOriginClient('audioInputDevices');
        },
        /**
         * [audioOutputDevices description]
         * @return {promise} [description]
         */
        audioOutputDevices: function() {
            return clientVoiceWrapper.sendToOriginClient('audioOutputDevices');
        },
        /**
         * [selectedAudioInputDevice description]
         * @return {promise} [description]
         */
        selectedAudioInputDevice: function() {
            return clientVoiceWrapper.sendToOriginClient('selectedAudioInputDevice');
        },
        /**
         * [selectedAudioOutputDevice description]
         * @return {promise} [description]
         */
        selectedAudioOutputDevice: function() {
            return clientVoiceWrapper.sendToOriginClient('selectedAudioOutputDevice');
        },
        /**
         * [networkQuality description]
         * @return {promise} [description]
         */
        networkQuality: function() {
            return clientVoiceWrapper.sendToOriginClient('networkQuality');
        },
        /**
         * [isInVoice description]
         * @return {Boolean} [description]
         */
        isInVoice: function() {
            return clientVoiceWrapper.sendToOriginClient('isInVoice');
        }
    };
});