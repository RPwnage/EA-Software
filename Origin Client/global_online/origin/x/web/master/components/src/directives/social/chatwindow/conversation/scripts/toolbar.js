(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginSocialChatwindowConversationToolbarCtrl($scope, $interval, RosterDataFactory, AuthFactory, VoiceFactory, UtilFactory) {

        var CONTEXT_NAMESPACE = 'origin-social-chatwindow-conversation-toolbar';
        var timer, seconds, minutes, hours, quality;
        var $element, $voiceLevelMeterBar0, $voiceLevelMeterBar1, $voiceLevelMeterBar2, $voiceLevelMeterBar3, $voiceLevelMeterBar4;
        var $networkQualityMeterBar0, $networkQualityMeterBar1, $networkQualityMeterBar2, $networkQualityMeterBar3;

        $scope.connectivityGoodLoc = UtilFactory.getLocalizedStr($scope.connectivityGoodStr, CONTEXT_NAMESPACE, 'connectivitygoodstr');
        $scope.connectivityOkLoc = UtilFactory.getLocalizedStr($scope.connectivityOkStr, CONTEXT_NAMESPACE, 'connectivityokstr');
        $scope.connectivityPoorLoc = UtilFactory.getLocalizedStr($scope.connectivityPoorStr, CONTEXT_NAMESPACE, 'connectivitypoorstr');
        $scope.disabledOffline = UtilFactory.getLocalizedStr($scope.disabledOfflineStr, CONTEXT_NAMESPACE, 'disabledofflinestr');
        $scope.disabledRemote = UtilFactory.getLocalizedStr($scope.disabledRemoteStr, CONTEXT_NAMESPACE, 'disabledremotestr');
        $scope.voiceChatSettingsLoc = UtilFactory.getLocalizedStr($scope.voiceChatSettingsStr, CONTEXT_NAMESPACE, 'voicechatsettingsstr');
        $scope.startVoiceCallLoc = UtilFactory.getLocalizedStr($scope.startVoiceCallStr, CONTEXT_NAMESPACE, 'startvoicecallstr');
        $scope.endVoiceCallLoc = UtilFactory.getLocalizedStr($scope.endVoiceCallStr, CONTEXT_NAMESPACE, 'endvoicecallstr');
        $scope.muteLoc = UtilFactory.getLocalizedStr($scope.muteStr, CONTEXT_NAMESPACE, 'mutestr');
        $scope.unmuteLoc = UtilFactory.getLocalizedStr($scope.unmuteStr, CONTEXT_NAMESPACE, 'unmutestr');

        $scope.offline = false;
        $scope.activeOneToOnePartnerOffline = false;
        $scope.isInvisible = false;
        $scope.inVoip = false;
        $scope.calling = false;
        $scope.isLocalMuted = false;
        $scope.calltimestr = '00:00';
        $scope.isVoiceSupported = Origin.voice.supported();
        $scope.voipButtonClicked = false;

        function startCallTimerAndQualityMeter() {
            if (!angular.isDefined(timer)) {
                seconds = 0;
                minutes = 0;
                hours = 0;

                timer = $interval(function () {

                    // call timer
                    ++seconds;
                    if (seconds >= 60) {
                        seconds = 0;
                        minutes++;
                        if (minutes >= 60) {
                            minutes = 0;
                            ++hours;
                        }
                    }

                    if (hours > 0) {
                        $scope.calltimestr = hours + ':' + (minutes < 10 ? ('0'+minutes) : minutes) + ':' + (seconds < 10 ? ('0'+seconds) : seconds);
                    }
                    else {
                        $scope.calltimestr = (minutes < 10 ? ('0'+minutes) : minutes) + ':' + (seconds < 10 ? ('0'+seconds) : seconds);
                    }

                    // quality meter
                    quality = Origin.voice.networkQuality();
                    setNetworkQualityMeter(quality);
                }, 1000);
            }
        }

        function endCallTimerAndQualityMeter() {
            if (angular.isDefined(timer)) {
                $interval.cancel(timer);
                timer = undefined;
                $scope.calltimestr = '00:00';
            }
        }

        function joinVoice() {
            Origin.voice.joinVoice($scope.conversation.id, $scope.conversation.participants);
        }

        function leaveVoice() {
            Origin.voice.leaveVoice($scope.conversation.id);
        }

        function reset() {
            $scope.calling = false;
            $scope.inVoip = false;
            $scope.isLocalMuted = false;
            $scope.voipButtonClicked = false;
            endCallTimerAndQualityMeter();
        }

        function updateVoiceState(isInit) {
            var voiceCallState = VoiceFactory.voiceCallState(),
                activeVoiceConversationId = VoiceFactory.getActiveVoiceConversationId(),
                outgoingVoiceConversationId = VoiceFactory.getOutgoingVoiceCallInfo().nucleusId,
                incomingVoiceConversationId = VoiceFactory.getIncomingVoiceCallInfo().nucleusId;
            if ( !isInit && voiceCallState !== undefined &&
                 ( (activeVoiceConversationId === 0 || activeVoiceConversationId === $scope.conversation.id) ||
                   incomingVoiceConversationId === $scope.conversation.id ||
                   outgoingVoiceConversationId === $scope.conversation.id
                 )
               )
            {
                switch (voiceCallState) {
                    case 'ENDED':
                    case 'ENDED_INCOMING':
                    case 'NOANSWER':
                    case 'MISSED':
                    case 'INACTIVITY':
                    case 'ENDED_UNEXPECTEDLY':
                    case 'CONNECT_ERROR':
                        reset();
                        break;
                    case 'OUTGOING':
                        $scope.calling = true;
                        $scope.inVoip = false;
                        $scope.voipButtonClicked = false;
                        break;
                    case 'STARTED':
                        $scope.calling = false;
                        $scope.inVoip = true;
                        $scope.voipButtonClicked = false;
                        startCallTimerAndQualityMeter();
                        break;
                }
            }
            else {
                reset();
            }
        }

        function updatePartnerOfflineState() {
            var nucleusId = $scope.conversation.id;
            RosterDataFactory.getFriendInfo(nucleusId).then(function(friend) {
                $scope.activeOneToOnePartnerOffline = (friend.presence.show === 'UNAVAILABLE');
            });
        }

        function onClientOnlineStateChanged(onlineObj) {
            $scope.offline = (onlineObj && !onlineObj.isOnline);
            updatePartnerOfflineState();
            $scope.$digest();
        }

        function onXmppConnect() {
            onClientOnlineStateChanged({isLoggedIn: true,
                                        isOnline:true});
        }

        function onXmppDisconnect() {
            onClientOnlineStateChanged({isLoggedIn: true,
                                        isOnline:false});
        }

        function handleVoiceCallEvent(voiceCallEventObj) {
            if ($scope.conversation.id === voiceCallEventObj.nucleusId) {
                updateVoiceState();
                $scope.$digest();
            }
        }

        function handlePresenceChange(presence) {
            $scope.activeOneToOnePartnerOffline = (presence.show === 'UNAVAILABLE');
            $scope.isInvisible = RosterDataFactory.getIsInvisible();
            $scope.$digest();
        }

        function onSetVoiceLevelMeter(value) { // value is 0 - 100
            if (VoiceFactory.voiceCallState() === 'LOCAL_MUTED') {
                value = 0;
            }

            $voiceLevelMeterBar0 = $element.find('.origin-social-conversation-toolbar-voicelevel-meter-bar0');
            $voiceLevelMeterBar1 = $element.find('.origin-social-conversation-toolbar-voicelevel-meter-bar1');
            $voiceLevelMeterBar2 = $element.find('.origin-social-conversation-toolbar-voicelevel-meter-bar2');
            $voiceLevelMeterBar3 = $element.find('.origin-social-conversation-toolbar-voicelevel-meter-bar3');
            $voiceLevelMeterBar4 = $element.find('.origin-social-conversation-toolbar-voicelevel-meter-bar4');
            $voiceLevelMeterBar0.toggleClass('lit', value >= 20 ? true : false);
            $voiceLevelMeterBar1.toggleClass('lit', value >= 40 ? true : false);
            $voiceLevelMeterBar2.toggleClass('lit', value >= 50 ? true : false);
            $voiceLevelMeterBar3.toggleClass('lit', value >= 60 ? true : false);
            $voiceLevelMeterBar4.toggleClass('lit', value >= 80 ? true : false);
        }

        function setNetworkQualityMeter(value) { // value is 0 - 3
            $networkQualityMeterBar0 = $element.find('.origin-social-conversation-toolbar-quality-meter-bar0');
            $networkQualityMeterBar1 = $element.find('.origin-social-conversation-toolbar-quality-meter-bar1');
            $networkQualityMeterBar2 = $element.find('.origin-social-conversation-toolbar-quality-meter-bar2');
            $networkQualityMeterBar3 = $element.find('.origin-social-conversation-toolbar-quality-meter-bar3');
            $networkQualityMeterBar0.attr('quality', value);
            $networkQualityMeterBar1.attr('quality', value);
            $networkQualityMeterBar2.attr('quality', value);
            $networkQualityMeterBar3.attr('quality', value);

            // add tooltip
            if (value === 3) {
                $scope.qualityTooltip = $scope.connectivityGoodLoc;
            }
            else if (value === 2) {
                $scope.qualityTooltip = $scope.connectivityOkLoc;
            }
            else {
                $scope.qualityTooltip = $scope.connectivityPoorLoc;
            }
        }

        function onDestroy() {
            AuthFactory.events.off('myauth:clientonlinestatechangeinitiated', onClientOnlineStateChanged);
            RosterDataFactory.events.off('xmppConnected', onXmppConnect);
            RosterDataFactory.events.off('xmppDisconnected', onXmppDisconnect);
            VoiceFactory.events.off('voiceCallEvent', handleVoiceCallEvent);
            VoiceFactory.events.off('voiceLevel', onSetVoiceLevelMeter);
            RosterDataFactory.events.off('socialPresenceChanged:' + nucleusId, handlePresenceChange);
        }
        $scope.$on('$destroy', onDestroy);

        // listen for connection change (for embedded)
        AuthFactory.events.on('myauth:clientonlinestatechangeinitiated', onClientOnlineStateChanged);

        // listen for xmpp connect/disconnect
        RosterDataFactory.events.on('xmppConnected', onXmppConnect);
        RosterDataFactory.events.on('xmppDisconnected', onXmppDisconnect);

        // listen for voice call events
        if ($scope.conversation.state === 'ONE_ON_ONE') {
            VoiceFactory.events.on('voiceCallEvent', handleVoiceCallEvent);
            VoiceFactory.events.on('voiceLevel', onSetVoiceLevelMeter);

            // listen for presence change of current 1:1 partner
            var nucleusId = $scope.conversation.participants[0];
            RosterDataFactory.events.on('socialPresenceChanged:' + nucleusId, handlePresenceChange);
        }

        this.onVoipButtonClick = function () {
            if ($scope.voipButtonClicked) {
                return;
            }

            $scope.voipButtonClicked = true;

            if (!$scope.inVoip && !$scope.calling) {
                joinVoice();
            }
            else {
                leaveVoice();
            }

            $scope.$digest();
        };

        this.onMicrophoneStatusClick = function () {
            $scope.isLocalMuted = !$scope.isLocalMuted;
            if ($scope.isLocalMuted) {
                Origin.voice.muteSelf();
            }
            else {
                Origin.voice.unmuteSelf();
            }
            $scope.$digest();
        };

        this.setElement = function ($el) {
            $element = $el;
        };

		updateVoiceState(true);
        updatePartnerOfflineState();
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialChatwindowConversationToolbar
     * @restrict E
     * @element ANY
     * @scope
     * @param {Object} conversation Javascript object representing the conversation
     * @param {LocalizedString} connectivitygoodstr "Network connectivity good"
     * @param {LocalizedString} connectivityokstr "Network connectivity OK"
     * @param {LocalizedString} connectivitypoorstr "Network connectivity poor"
     * @param {String} disabledOfflineStr Message to display when the voice call button is disabled due to the client being offline.
     * @param {String} disabledRemoteStr Message to display when the voice call button is disabled due to none of the remote chat users having voice chat capability.
     * @param {LocalizedString} voicechatsettingsstr "Voice chat settings"
     * @param {LocalizedString} startvoicecallstr "Start voice call"
     * @param {LocalizedString} endvoicecallstr "End voice call"
     * @param {LocalizedString} mutestr "Mute"
     * @param {LocalizedString} unmutestr "Unmute"
     * @description
     *
     * origin chatwindow -> conversation -> toolbar
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-social-chatwindow-conversation-toolbar
     *            conversation="conversation"
     *            connectivitygoodstr "Network connectivity good"
     *            connectivityokstr "Network connectivity OK"
     *            connectivitypoorstr "Network connectivity poor"
     *            disabledOfflineStr="Voice chat is disabled because the client is offline."
     *            disabledRemoteStr="Voice chat is disabled because none of the remote participants can voice chat."
     *            voicechatsettingsstr="Voice chat settings"
     *            startvoicecallstr="Start voice call"
     *           *            voicechatsettingsstr="Voice chat settings"
     *            startvoicecallstr="Start voice call"
     *            endvoicecallstr="End voice call"
     *            mutestr="Mute"
     *            unmutestr="Unmute"
     *         ></origin-social-chatwindow-conversation-toolbar>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialChatwindowConversationToolbarCtrl', OriginSocialChatwindowConversationToolbarCtrl)
        .directive('originSocialChatwindowConversationToolbar', function(ComponentsConfigFactory, NavigationFactory) {

            return {
                restrict: 'E',
                controller: 'OriginSocialChatwindowConversationToolbarCtrl',
                scope: {
                    conversation: '=',
                    connectivityGoodStr: '@connectivitygoodstr',
                    connectivityOkStr: '@connectivityokstr',
                    connectivityPoorStr: '@connectivitypoorstr',
                    voiceChatSettingsStr: '@voicechatsettingsstr',
                    startVoiceCallStr: '@startvoicecallstr',
                    endVoiceCallStr: '@endvoicecallstr',
                    muteStr: '@mutestr',
                    unmuteStr: '@unmutestr'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/chatwindow/conversation/views/toolbar.html'),
                link: function (scope, element, attrs, ctrl) {
                    scope = scope;
                    attrs = attrs;

                    ctrl.setElement($(element).find('.origin-social-conversation-toolbar'));

                    // clicked on the voip button
                    $(element).on('click', '.origin-social-conversation-toolbar-voipicon', function () {
                        if (!$(this).hasClass('disabled')) {
                            ctrl.onVoipButtonClick();
                        }
                    });

                    // clicked on the microphone status button
                    $(element).on('click', '.origin-social-conversation-toolbar-micstatus', function () {
                        ctrl.onMicrophoneStatusClick();
                    });

                    // clicked on the settings button
                    $(element).on('click', '.origin-social-conversation-toolbar-settingsicon', function () {
                        NavigationFactory.goSettingsPage('voice');
                    });

                    // clicked on the invite button
                    $(element).on('click', '.origin-social-conversation-toolbar-inviteicon', function () {
                        window.alert('FEATURE NOT IMPLEMENTED - should bring up invite window');
                    });
                }
            };

        });
}());

