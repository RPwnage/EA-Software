(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginSocialChatwindowConversationBannerCtrl($scope, RosterDataFactory, UtilFactory, VoiceFactory) {

	    var CONTEXT_NAMESPACE = 'origin-social-chatwindow-conversation-banner';

        $scope.playingLoc = UtilFactory.getLocalizedStr($scope.playingStr, CONTEXT_NAMESPACE, 'playingstr');
        $scope.broadcastingLoc = UtilFactory.getLocalizedStr($scope.broadcastingStr, CONTEXT_NAMESPACE, 'broadcastingstr');
        $scope.joinLoc = UtilFactory.getLocalizedStr($scope.joinStr, CONTEXT_NAMESPACE, 'joinstr');
        $scope.orLoc = UtilFactory.getLocalizedStr($scope.orStr, CONTEXT_NAMESPACE, 'orstr');
        $scope.watchLoc = UtilFactory.getLocalizedStr($scope.watchStr, CONTEXT_NAMESPACE, 'watchstr');
        $scope.cancelLoc = UtilFactory.getLocalizedStr($scope.cancelStr, CONTEXT_NAMESPACE, 'cancelstr');
        $scope.incomingcallLoc = UtilFactory.getLocalizedStr($scope.incomingcallStr, CONTEXT_NAMESPACE, 'incomingcallstr');

        var nucleusId;

        $scope.state = {
            voice : false,
            playingGame: false
        };

        $scope.activeOneToOnePartner = {};
        $scope.voiceCallState = 'NONE';

        this.getIncomingNucleusId = function() {
            return VoiceFactory.getIncomingVoiceCallInfo().nucleusId;
        };

        this.getOutgoingNucleusId = function() {
            return VoiceFactory.getOutgoingVoiceCallInfo().nucleusId;
        };

        function translatePresence(presence) {
            $scope.activeOneToOnePartner.joinable = false;
            $scope.activeOneToOnePartner.ingame = false;
            $scope.activeOneToOnePartner.broadcasting = false;
            $scope.activeOneToOnePartner.hyperlinkGameTitle = true;
            if (presence.show === 'ONLINE') {
                if (presence.gameActivity && presence.gameActivity.title !== undefined && presence.gameActivity.title !=='') {
                    // If we have a gameActivity object and a title we must be playing a game.
                    $scope.activeOneToOnePartner.ingame = true;

                    if ($scope.activeOneToOnePartner.presence.gameActivity.productId.indexOf('NOG_') > -1) {
                        // NOG
                        $scope.activeOneToOnePartner.hyperlinkGameTitle = false;
                    }

                    if ($scope.activeOneToOnePartner.presence.gameActivity.title.indexOf('Unknown game [R&D Mode]') > -1) {
                        // Embargoed title
                        $scope.activeOneToOnePartner.hyperlinkGameTitle = false;
                    }

                    if (presence.gameActivity.joinable){
                        $scope.activeOneToOnePartner.joinable = true;
                    }

					// If we have a twitch presence we must be broadcasting
					if (!!presence.gameActivity.twitchPresence && presence.gameActivity.twitchPresence.length) {
						$scope.activeOneToOnePartner.broadcasting = true;
					}
                }
            }
        }

        function handlePresenceChange(presence) {
			$scope.activeOneToOnePartner.presence = presence;
			translatePresence(presence);
			updateState();
			$scope.$digest();
        }

        function resetVoiceCallInfo(nucleusId) {
            if (nucleusId === VoiceFactory.getActiveVoiceConversationId()) {
                VoiceFactory.setActiveVoiceConversationId(0);
            }

            if (VoiceFactory.getIncomingVoiceCallInfo().nucleusId !== 0) {
                if (nucleusId === VoiceFactory.getIncomingVoiceCallInfo().nucleusId) {
                    VoiceFactory.setIncomingVoiceCallInfo(0, '');
                }
                else {
                    $scope.state.voiceCall = true;
                    $scope.voiceCallState = 'INCOMING';
                }
            }

            if (nucleusId === VoiceFactory.getOutgoingVoiceCallInfo().nucleusId) {
                VoiceFactory.setOutgoingVoiceCallInfo(0, '');
            }
        }

        /*function updateRinger() {
            console.log('SG: in updateRinger');
            var voiceCallState = $scope.voiceCallState;
            if (voiceCallState === 'INCOMING') {
                Origin.voice.playIncomingRing();
            }
            else {
                Origin.voice.stopIncomingRing();
            }

            if (voiceCallState === 'OUTGOING') {
                Origin.voice.playOutgoingRing();
            }
            else {
                Origin.voice.stopOutgoingRing();
            }
        }*/

        function updateVoiceState(nucleusId, originId) {
            var outgoingOriginId = '',
                incomingOriginId = '',
                voiceCallState = VoiceFactory.voiceCallState();
            if (voiceCallState !== undefined) {
                switch (voiceCallState) {
                    case 'ENDED':
                    case 'STARTED':
                    case 'NOANSWER':
                    case 'MISSED':
                    case 'INACTIVITY':
                    case 'ENDED_INCOMING':
                        $scope.state.voiceCall = false;
                        $scope.voiceCallState = 'NONE';
                        resetVoiceCallInfo(nucleusId); // might modify '$scope.state.voiceCall' and '$scope.voiceCallState'
                        break;

                    case 'LOCAL_MUTED':
                    case 'LOCAL_UNMUTED':
                        // do nothing;
                        break;

                    case 'ENDED_UNEXPECTEDLY':
                    case "CONNECT_ERROR":
                        resetVoiceCallInfo(nucleusId); // might modify '$scope.state.voiceCall' and '$scope.voiceCallState'
                        break;

                    case 'OUTGOING':
                    case 'INCOMING':
                        if (voiceCallState === 'INCOMING') {
                            if(VoiceFactory.getIncomingVoiceCallInfo().nucleusId === 0) {
                                VoiceFactory.setIncomingVoiceCallInfo(nucleusId, originId);
                            }
                            incomingOriginId = VoiceFactory.getIncomingVoiceCallInfo().originId;
                            $scope.iscallingLoc = UtilFactory.getLocalizedStr($scope.iscallingStr, CONTEXT_NAMESPACE, 'iscallingstr', {'%username%': incomingOriginId});
                        }
                        if (voiceCallState === 'OUTGOING') {
                            if(VoiceFactory.getOutgoingVoiceCallInfo().nucleusId === 0) {
                                VoiceFactory.setOutgoingVoiceCallInfo(nucleusId, originId);
                            }
                            outgoingOriginId = VoiceFactory.getOutgoingVoiceCallInfo().originId;
                            $scope.callingLoc = UtilFactory.getLocalizedStr($scope.callingStr, CONTEXT_NAMESPACE, 'callingstr', {'%username%': outgoingOriginId});
                        }

                        // outgoing banner supercedes incoming banner
                        $scope.state.voiceCall = true;
                        if ((voiceCallState === 'INCOMING') && (VoiceFactory.getOutgoingVoiceCallInfo().nucleusId !== 0)) {
                            $scope.voiceCallState = 'OUTGOING';
                            outgoingOriginId = VoiceFactory.getOutgoingVoiceCallInfo().originId;
                            $scope.callingLoc = UtilFactory.getLocalizedStr($scope.callingStr, CONTEXT_NAMESPACE, 'callingstr', {'%username%': outgoingOriginId});
                        }
                        else {
                            $scope.voiceCallState = voiceCallState;
                        }
                        break;

                    default:
                        break;
                }
                //updateRinger();
           }
       }

        function handleVoiceCallEvent(voiceCallEventObj) {
            var nucleusId = voiceCallEventObj.nucleusId;

            RosterDataFactory.getFriendInfo(nucleusId).then(function(friend) {
                updateVoiceState(nucleusId, friend.originId);
                $scope.$digest();
            });
        }

        function onDestroy() {
            RosterDataFactory.events.off('socialPresenceChanged:' + nucleusId, handlePresenceChange);
            VoiceFactory.events.off('voiceCallEvent', handleVoiceCallEvent);
        }

        function registerForEvents() {
            var nucleusId = $scope.conversation.participants[0];
            RosterDataFactory.events.on('socialPresenceChanged:' + nucleusId, handlePresenceChange);
            VoiceFactory.events.on('voiceCallEvent', handleVoiceCallEvent);
            $scope.$on('$destroy', onDestroy);
        }

        function updateState() {
            if ($scope.conversation.state === 'ONE_ON_ONE') {
                if (!$scope.state.playingGame && $scope.activeOneToOnePartner.ingame) {
                    $scope.state.playingGame = true;
                }
                else if ($scope.state.playingGame && !$scope.activeOneToOnePartner.ingame) {
                    $scope.state.playingGame = false;
                }
            }
        }

        if ($scope.conversation.state === 'ONE_ON_ONE') {
            registerForEvents();
            nucleusId = $scope.conversation.participants[0];
            RosterDataFactory.getFriendInfo(nucleusId).then(function(user) {
                $scope.activeOneToOnePartner.presence = user.presence;
                if (!!user.presence) {
                    translatePresence(user.presence);
                }
                updateState();
                updateVoiceState(nucleusId.toString(), user.originId);
                $scope.$digest();
            });
        }
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialChatwindowConversationBanner
     * @restrict E
     * @element ANY
     * @scope
     * @param {Object} conversation Javascript object representing the conversation
     * @param {LocalizedString} playingstr "Playing"
     * @param {LocalizedString} broadcastingstr "Broadcasting"
     * @param {LocalizedString} joinstr "Join"
     * @param {LocalizedString} orstr "or"
     * @param {LocalizedString} watchstr "Watch"
     * @param {LocalizedString} callingstr "Calling %username%..."
     * @param {LocalizedString} cancelstr "Cancel"
     * @param {LocalizedString} incomingcallstr "Incoming call"
     * @param {LocalizedString} iscallingstr "%username% is calling"
     * @description
     *
     * origin chatwindow -> conversation -> banner
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-social-chatwindow-conversation-banner
     *            conversation="conversation"
     *            playingstr="Playing"
     *            broadcastingstr="Broadcasting"
     *            joinstr="Join"
     *            orstr="or"
     *            watchstr="Watch"
     *            callingstr="Calling %username%..."
     *            cancelstr="Cancel"
     *            incomingcallstr="Incoming call"
     *            iscallingstr="%username% is calling"
     *         ></origin-social-chatwindow-conversation-banner>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialChatwindowConversationBannerCtrl', OriginSocialChatwindowConversationBannerCtrl)
        .directive('originSocialChatwindowConversationBanner', function (ComponentsConfigFactory, NavigationFactory, JoinGameFactory) {

            return {
                restrict: 'E',
                controller: 'OriginSocialChatwindowConversationBannerCtrl',
                scope: {
                    conversation: '=',
                    playingStr: '@playingstr',
                    broadcastingStr: '@broadcastingstr',
                    joinStr: '@joinstr',
                    orStr: '@orstr',
                    watchStr: '@watchstr',
                    callingStr: '@callingstr',
                    cancelStr: '@cancelstr',
                    incomingcallStr: '@incomingcallstr',
                    iscallingStr: '@iscallingstr'
                },
                link: function(scope, element, attrs, ctrl) {

                    attrs = attrs;
                    ctrl = ctrl;

                    // Clicked on a product link
                    $(element).on('click', '.origin-social-chatwindow-conversation-banner-productlink', function(event) {
                        var productId = $(event.target).attr('data-product-id');
                        var multiplayerId = $(event.target).attr('data-multiplayer-id');
                        if (!!productId && !!multiplayerId) {
                            NavigationFactory.goProductDetailsWithMultiplayerId(productId, multiplayerId);
                        }
                        else if (!!productId) {
                            NavigationFactory.goProductDetails(productId);
                        }
                    });

                    // Clicked "Join"
                    $(element).on('click', '.origin-social-chatwindow-conversation-banner-join', function(event) {
                        event = event;

                        var nucleusId = scope.conversation.participants[0];
                        var productId = scope.activeOneToOnePartner.presence.gameActivity.productId;
                        var multiplayerId = scope.activeOneToOnePartner.presence.gameActivity.multiplayerId;
                        var gameTitle = scope.activeOneToOnePartner.presence.gameActivity.title;
                        JoinGameFactory.joinFriendsGame(nucleusId, productId, multiplayerId, gameTitle);

                    });

                    // Clicked "Watch"
                    $(element).on('click', '.origin-social-chatwindow-conversation-banner-watch', function(event) {
                        var url = $(event.target).attr('data-broadcast-url');
                        if (!!url) {
                            NavigationFactory.asyncOpenUrl(url);
                        }
                    });

                    // Clicked 'Cancel' on a voice call
                    angular.element(element).on('click', '.origin-social-chatwindow-conversation-banner-voicecall-cancel', function(event) {
                        event = event;
                        Origin.voice.leaveVoice(ctrl.getOutgoingNucleusId());
                    });

                    // Clicked 'Answer' on a voice call
                    angular.element(element).on('click', '.origin-social-chatwindow-conversation-banner-voicecall-action-link-answer', function (event) {
                        event = event;
                        Origin.voice.answerCall(ctrl.getIncomingNucleusId());
                    });

                    // Clicked 'Ignore' on a voice call
                    angular.element(element).on('click', '.origin-social-chatwindow-conversation-banner-voicecall-action-link-ignore', function (event) {
                        event = event;
                        Origin.voice.ignoreCall(ctrl.getIncomingNucleusId());
                    });
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/chatwindow/conversation/views/banner.html'),
            };

        });
}());

