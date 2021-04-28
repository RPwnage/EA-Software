(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginSocialChatwindowConversationToolbarCtrl($scope, $timeout, RosterDataFactory, AuthFactory, VoiceFactory, UtilFactory, ObserverFactory, ConversationFactory, moment) {

        var CONTEXT_NAMESPACE = 'origin-social-chatwindow-conversation-toolbar';
        var $element, $micIcon, $voiceLevelMeterBar0, $voiceLevelMeterBar1, $voiceLevelMeterBar2, $voiceLevelMeterBar3, $voiceLevelMeterBar4;
        var $networkQualityMeterBar0, $networkQualityMeterBar1, $networkQualityMeterBar2, $networkQualityMeterBar3;
        var self = this;
        var conversationIsVoiceSupportedRemotelyObservable;
        var voiceCallDurationObserver;

        function isCurrentConversationInVoip() {
            var activeVoiceConversationId = VoiceFactory.activeVoiceConversationId();
            return VoiceFactory.voiceCallObservable().data.isInVoice && (activeVoiceConversationId === Number($scope.conversation.id));
        }

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
        $scope.voipIconCallText = $scope.conversation.remoteVoice.data.isSupported ? $scope.startVoiceCallLoc : $scope.disabledRemote;
        $scope.inVoip = isCurrentConversationInVoip();
        $scope.calling = VoiceFactory.voiceCallObservable().data.isCalling;
        $scope.isLocalMuted = VoiceFactory.voiceCallObservable().data.isLocalMuted;
        $scope.calltimestr = '00:00';
        $scope.isVoiceSupported = Origin.voice.supported();
        $scope.voipButtonClicked = false;
        $scope.inProgressBannerActive = false;
		
		RosterDataFactory.getRosterUser(Origin.user.userPid()).then(function(user) {
			if (!!user) {
				ObserverFactory.create(user._presence)
					.defaultTo({jid: Origin.user.userPid(), presenceType: '', show: 'UNAVAILABLE', gameActivity: {}})
					.attachToScope($scope, 'selfPresence');
			}
		});

        var nucleusId = $scope.conversation.participants[0];
        RosterDataFactory.getRosterUser(nucleusId).then(function(user) {
            if (!!user) {
                ObserverFactory.create(user._presence)
                    .defaultTo({jid: nucleusId, presenceType: '', show: 'UNAVAILABLE', gameActivity: {}})
                    .attachToScope($scope, 'chatPartnerPresence');
            }
        });

        function onVoiceCallChange() {
            $scope.inVoip = isCurrentConversationInVoip();
            $scope.calling = VoiceFactory.voiceCallObservable().data.isCalling;
            $scope.isLocalMuted = VoiceFactory.voiceCallObservable().data.isLocalMuted;
        }
        VoiceFactory.voiceCallObservable().addObserver(onVoiceCallChange);

        function startCallDurationUpdate() {
            voiceCallDurationObserver = VoiceFactory.voiceCallDurationObservable().addObserver(function() {
                $timeout(function() {
                    // anything you want can go here and will safely be run on the next digest.
                    var time = moment.duration(VoiceFactory.voiceCallDurationObservable().data.seconds, 'seconds');
                    $scope.calltimestr = time.format('hh:mm:ss', {trim: false});

                    // quality meter
                    Origin.voice.networkQuality().then(setNetworkQualityMeter);
                });
            });
        }

        function endCallDurationUpdate() {
            if (angular.isDefined(voiceCallDurationObserver)) {
                voiceCallDurationObserver.detach();
                voiceCallDurationObserver = undefined;
            }
            $scope.calltimestr = '00:00:00';
        }

        function joinVoice() {
            $scope.inVoip = isCurrentConversationInVoip();
            if($scope.conversation.id) {
                if(!$scope.inVoip && VoiceFactory.activeVoiceConversationId() === 0 ){
                    // not yet in a call
                    VoiceFactory.setActiveVoiceConversationId($scope.conversation.id);
                    Origin.voice.joinVoice($scope.conversation.id, $scope.conversation.participants);
                }
                else {
                    // in a call - show incall dialog
                    ConversationFactory.showInProgressCallBanner($scope.conversation.id);
                    $scope.inProgressBannerActive = true;
                    $scope.voipButtonClicked = false;
                }
            }
        }

        function leaveVoice() {
            if($scope.conversation.id) {
                Origin.voice.leaveVoice($scope.conversation.id);
                if($scope.inProgressBannerActive === true) {
                    ConversationFactory.hideInProgressCallBanner($scope.conversation.id);
                    $scope.inProgressBannerActive = false;
                }
                $scope.voipButtonClicked = false;
            }
        }

        function reset() {
            $scope.inVoip = isCurrentConversationInVoip();
            $scope.calling = VoiceFactory.voiceCallObservable().data.isCalling;
            $scope.isLocalMuted = VoiceFactory.voiceCallObservable().data.isLocalMuted;
            $scope.voipButtonClicked = false;
            endCallDurationUpdate();
        }

        function updateVoiceState(isInit) {
            var nucleusId = $scope.conversation.participants[0],
                callInfo = VoiceFactory.voiceCallInfoForConversation(nucleusId),
                voiceCallState = callInfo.state,
                activeVoiceConversationId = VoiceFactory.activeVoiceConversationId(),
                isOutgoing = (callInfo.state === 'OUTGOING'),
                isIncoming = (callInfo.state === 'INCOMING');

            if ( !isInit && voiceCallState !== undefined &&
                 ( (activeVoiceConversationId === 0 || activeVoiceConversationId === Number($scope.conversation.id)) ||
                   isIncoming ||
                   isOutgoing
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
                        $scope.inProgressBannerActive = false;
                        break;
                    case 'OUTGOING':
                        $scope.calling = true;
                        $scope.voipButtonClicked = false;
                        break;
                    case 'STARTED':
                        $scope.voipButtonClicked = false;
                        startCallDurationUpdate();
                        setTimeout(function() {
                            Origin.client.settings.readSetting("EnablePushToTalk").then(self.updateMicForPushToTalk);
                        }, 0);
                        break;
                    case 'PTT_ACTIVE':
                    case 'PTT_INACTIVE':
                        $micIcon = $element.find('.origin-social-conversation-toolbar-micstatus');
                        if (voiceCallState === 'PTT_ACTIVE') {
                            $micIcon.addClass("push-to-talk-active");
                        }
                        else {
                            $micIcon.removeClass("push-to-talk-active");
                        }
                        break;
                }
            }
            else {
                reset();

                if ($scope.inVoip) {
                    startCallDurationUpdate();
                }
            }
        }

        function handleVoiceCallEvent(voiceCallEventObj) {
            if ($scope.conversation.id === voiceCallEventObj.nucleusId) {
                updateVoiceState();
                $scope.$digest();
            }
        }

        function onSetVoiceLevelMeter(value) { // value is 0 - 100
            if (VoiceFactory.voiceCallInfoForConversation(Number($scope.conversation.id)).state === 'LOCAL_MUTED') {
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

        this.updateMicForPushToTalk = function(enabled) {
            $micIcon = $element.find('.origin-social-conversation-toolbar-micstatus');
            if (enabled) {
                $micIcon.addClass("push-to-talk-mode");
            }
            else {
                $micIcon.removeClass("push-to-talk-mode");
                $micIcon.removeClass("push-to-talk-active");
            }
        };

        function onUpdateSettings(name, value) {
            if (name === 'EnablePushToTalk') {
                self.updateMicForPushToTalk(value === 'true');
            }
        }

        function inProgressBannerHidden() {
            $scope.inProgressBannerActive = false;
        }


        function onRemoteVoiceChange() {
            var newValue = $scope.conversation.remoteVoice.data.isSupported ? $scope.startVoiceCallLoc : $scope.disabledRemote;
            $element.find('span.origin-social-conversation-toolbar-voipicon-call').trigger('tooltip:update', { 'old': $scope.voipIconCallText, 'new': newValue });
            $scope.voipIconCallText = newValue;
        }

        function onDestroy() {
            VoiceFactory.events.off('voiceCallEvent', handleVoiceCallEvent);
            VoiceFactory.events.off('voiceLevel', onSetVoiceLevelMeter);
            Origin.events.off(Origin.events.CLIENT_SETTINGS_UPDATESETTINGS, onUpdateSettings);
            ConversationFactory.events.off('inProgressCallBannerHidden:' + nucleusId, inProgressBannerHidden);
            VoiceFactory.voiceCallObservable().removeObserver(onVoiceCallChange);
            $scope.conversation.remoteVoice.removeObserver(onRemoteVoiceChange);

        }
        $scope.$on('$destroy', onDestroy);

        // listen for xmpp connect/disconnect
        ObserverFactory.create(RosterDataFactory.getXmppConnectionWatch())
            .defaultTo({isConnected: Origin.xmpp.isConnected()})
            .attachToScope($scope, 'xmppConnection');

        // listen for changes to the conversation isVoiceSupportedRemotely state
		conversationIsVoiceSupportedRemotelyObservable =
			ObserverFactory.create($scope.conversation.remoteVoice)
			.defaultTo({isSupported: false})
			.attachToScope($scope, 'remoteVoice');

        $scope.conversation.remoteVoice.addObserver(onRemoteVoiceChange);

        // listen for voice call events
        if ($scope.conversation.state === 'ONE_ON_ONE') {
            VoiceFactory.events.on('voiceCallEvent', handleVoiceCallEvent);
            VoiceFactory.events.on('voiceLevel', onSetVoiceLevelMeter);
        }

        // listen for settings update event
        Origin.events.on(Origin.events.CLIENT_SETTINGS_UPDATESETTINGS, onUpdateSettings);

        // listen for InProgress call banner being hidden
        ConversationFactory.events.on('inProgressCallBannerHidden:' + nucleusId, inProgressBannerHidden);

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
            $scope.isLocalMuted = VoiceFactory.toggleMute();
            $scope.$digest();
        };

        this.setElement = function ($el) {
            $element = $el;
        };

		updateVoiceState(true);
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
     * @param {LocalizedString} disabledofflinestr Message to display when the voice call button is disabled due to the client being offline.
     * @param {LocalizedString} disabledremotestr Message to display when the voice call button is disabled due to none of the remote chat users having voice chat capability.
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
     *            disabledofflinestr="Voice chat is disabled because the client is offline."
     *            disabledremotestr="Voice chat is disabled because none of the remote participants can voice chat."
     *            voicechatsettingsstr="Voice chat settings"
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
                    unmuteStr: '@unmutestr',
                    disabledOfflineStr: '@disabledofflinestr',
                    disabledRemoteStr: '@disabledremotestr'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/chatwindow/conversation/views/toolbar.html'),
                link: function (scope, element, attrs, ctrl) {
                    scope = scope;
                    attrs = attrs;

                    ctrl.setElement($(element).find('.origin-social-conversation-toolbar'));

                    // clicked on the voip button
                    $(element).on('click', '.origin-social-conversation-toolbar-voipicon', function () {
                        if (!$(this).hasClass('isDisabled')) {
                            ctrl.onVoipButtonClick();
                        }
                    });

                    // clicked on the microphone status button
                    $(element).on('click', '.origin-social-conversation-toolbar-micstatus', function () {
                        ctrl.onMicrophoneStatusClick();
                    });

                    // clicked on the settings button
                    $(element).on('click', '.origin-social-conversation-toolbar-settingsicon', function () {
                        if (Origin.client.oig.IGOIsActive()) {
                            // Open OIG settings window
                            NavigationFactory.openIGOSPA('SETTINGS', 'voice');
                        }else {
                            NavigationFactory.goSettingsPage('voice');
                        }
                    });

                    // clicked on the invite button
                    $(element).on('click', '.origin-social-conversation-toolbar-inviteicon', function () {
                        window.alert('FEATURE NOT IMPLEMENTED - should bring up invite window');
                    });
                }
            };

        });
}());

