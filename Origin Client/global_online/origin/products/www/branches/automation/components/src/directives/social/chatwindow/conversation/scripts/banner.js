(function() {

    'use strict';

    /**
     * The controller
     */
    function OriginSocialChatwindowConversationBannerCtrl($scope, $timeout, RosterDataFactory, UtilFactory, VoiceFactory, ConversationFactory, GamesDataFactory, ComponentsLogFactory, ObserverFactory) {

        var CONTEXT_NAMESPACE = 'origin-social-chatwindow-conversation-banner';

        $scope.playingLoc = UtilFactory.getLocalizedStr($scope.playingStr, CONTEXT_NAMESPACE, 'playingstr');
        $scope.playingInGroupLoc = UtilFactory.getLocalizedStr($scope.playingInGroupLoc, CONTEXT_NAMESPACE, 'playingingroupstr');
        $scope.playingInGroupWithNameLoc = UtilFactory.getLocalizedStr($scope.playingInGroupWithNameLoc, CONTEXT_NAMESPACE, 'playingingroupwithnamestr');
        $scope.broadcastingLoc = UtilFactory.getLocalizedStr($scope.broadcastingStr, CONTEXT_NAMESPACE, 'broadcastingstr');
        $scope.joinGameLoc = UtilFactory.getLocalizedStr($scope.joinGameLoc, CONTEXT_NAMESPACE, 'joingamecta');
        $scope.joinGroupLoc = UtilFactory.getLocalizedStr($scope.joinGroupLoc, CONTEXT_NAMESPACE, 'joingroupcta');
        $scope.joinGameAndGroupLoc = UtilFactory.getLocalizedStr($scope.joinGameAndGroupLoc, CONTEXT_NAMESPACE, 'joingameandgroupcta');
        $scope.orLoc = UtilFactory.getLocalizedStr($scope.orStr, CONTEXT_NAMESPACE, 'orstr');
        $scope.watchLoc = UtilFactory.getLocalizedStr($scope.watchStr, CONTEXT_NAMESPACE, 'watchstr');
        $scope.ignoreLoc = UtilFactory.getLocalizedStr($scope.ignoreStr, CONTEXT_NAMESPACE, 'ignorestr');
        $scope.cancelLoc = UtilFactory.getLocalizedStr($scope.cancelStr, CONTEXT_NAMESPACE, 'cancelstr');
        $scope.incomingcallLoc = UtilFactory.getLocalizedStr($scope.incomingcallStr, CONTEXT_NAMESPACE, 'incomingcallstr');
        $scope.inProgressCallUserStartedDescriptionLoc = UtilFactory.getLocalizedStr($scope.inProgressCallUserStartedDescriptionStr, CONTEXT_NAMESPACE, 'inprogresscalluserstartedndescriptionstr');
        $scope.inProgressCallFriendStartedDescriptionLoc = UtilFactory.getLocalizedStr($scope.inProgressCallFriendStartedDescriptionStr, CONTEXT_NAMESPACE, 'inprogresscallfriendstarteddescriptionstr');
        $scope.answerCallLoc = UtilFactory.getLocalizedStr($scope.answerCallStr, CONTEXT_NAMESPACE, 'answercallstr');
        $scope.ignoreCallLoc = UtilFactory.getLocalizedStr($scope.ignoreCallStr, CONTEXT_NAMESPACE, 'ignorecallstr');
        $scope.currentUserPresence = RosterDataFactory.getCurrentUserPresence();

        function isCurrentConversationInVoip() {
            var activeVoiceConversationId = VoiceFactory.activeVoiceConversationId();
            return VoiceFactory.voiceCallObservable().data.isInVoice && (activeVoiceConversationId === $scope.conversation.id);
        }
        $scope.callInProgress = isCurrentConversationInVoip();
        $scope.anyCallInProgress = VoiceFactory.activeVoiceConversationId();
        $scope.callInProgressOverride = false;

        $scope.joinGameStrings = {
            unableToJoinText: UtilFactory.getLocalizedStr($scope.joinGameUnableText, CONTEXT_NAMESPACE, 'join-game-unable-text'),
            notOwnedText: UtilFactory.getLocalizedStr($scope.joinGameNotOwnedText, CONTEXT_NAMESPACE, 'join-game-not-owned-text'),
            viewInStoreText: UtilFactory.getLocalizedStr($scope.joinGameViewInStoreButtonText, CONTEXT_NAMESPACE, 'join-game-view-in-store-button-text'),
            closeText: UtilFactory.getLocalizedStr($scope.joinGameCloseButtonText, CONTEXT_NAMESPACE, 'join-game-close-button-text'),
            notInstalledText: UtilFactory.getLocalizedStr($scope.joinGameNotInstalledText, CONTEXT_NAMESPACE, 'join-game-not-installed-text'),
            downloadText: UtilFactory.getLocalizedStr($scope.joinGameDownloadButtonText, CONTEXT_NAMESPACE, 'join-game-download-button-text')
        };

        RosterDataFactory.getRosterUser(Origin.user.userPid()).then(function(user) {
            if (!!user) {
                ObserverFactory.create(user._presence)
                    .defaultTo({jid: Origin.user.userPid(), presenceType: '', show: 'UNAVAILABLE', gameActivity: {}})
                    .attachToScope($scope, 'selfPresence');
            }
        });

        function onVoiceCallChange() {
            $scope.inVoip = isCurrentConversationInVoip();
            $scope.calling = VoiceFactory.voiceCallObservable().data.isCalling;
            $scope.isLocalMuted = VoiceFactory.voiceCallObservable().data.isLocalMuted;
        }

        VoiceFactory.voiceCallObservable().addObserver(onVoiceCallChange);

        var originId;

        $scope.gameInvite = {};

        $scope.state = {
            voice: false,
            invited: false,
            playingGame: false
        };

        $scope.voiceCallState = 'NONE';

        $scope.showGameTitleHyperlink = function(presence) {
            var show = true;

            if (!!presence && !!presence.gameActivity) {
                if (!!presence.gameActivity.productId && presence.gameActivity.productId.indexOf('NOG_') > -1) {
                    // NOG
                    show = false;
                } else if (RosterDataFactory.isEmbargoedPresence(presence)) {
                    // Embargoed title
                    show = false;
                }
            }

            return show;
        };

        function resetVoiceCallInfoForConversation(nucleusId) {
            if (nucleusId === VoiceFactory.activeVoiceConversationId()) {
                VoiceFactory.setActiveVoiceConversationId(0);
            }

            VoiceFactory.voiceCallInfoForConversation(nucleusId).state = 'NONE';
            $scope.state.voiceCall = false;
            $scope.voiceCallState = 'NONE';
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

        function updateVoiceState(nucleusId, originId, event) {
            var voiceCallState = VoiceFactory.voiceCallInfoForConversation(nucleusId).state;
            $scope.anyCallInProgress = VoiceFactory.activeVoiceConversationId();
            switch (event) {
                case 'STARTED':
                    $scope.state.voiceCall = false;
                    $scope.callInProgressOverride = false;
                    $scope.voiceCallState = 'NONE';
                    VoiceFactory.voiceCallInfoForConversation(nucleusId).state = event;
                    break;

                case 'ENDED':
                case 'NOANSWER':
                case 'MISSED':
                case 'INACTIVITY':
                case 'ENDED_INCOMING':
                    $scope.callInProgressOverride = false;
                    $scope.state.voiceCall = false;
                    $scope.voiceCallState = 'NONE';
                    resetVoiceCallInfoForConversation(nucleusId); // might modify '$scope.state.voiceCall' and '$scope.voiceCallState'
                    break;

                case 'LOCAL_MUTED':
                case 'LOCAL_UNMUTED':
                case 'PTT_ACTIVE':
                case 'PTT_INACTIVE':
                    // do nothing;
                    break;

                case 'ENDED_UNEXPECTEDLY':
                case "CONNECT_ERROR":
                    resetVoiceCallInfoForConversation(nucleusId); // might modify '$scope.state.voiceCall' and '$scope.voiceCallState'
                    break;

                case 'OUTGOING':
                case 'INCOMING':
                    if (event === 'INCOMING') {
                        if (voiceCallState === 'OUTGOING') {
                            $scope.voiceCallState = 'OUTGOING';
                            $scope.callingLoc = UtilFactory.getLocalizedStr($scope.callingStr, CONTEXT_NAMESPACE, 'callingstr', {
                                '%username%': VoiceFactory.voiceCallInfoForConversation(nucleusId).originId
                            });
                            if(!$scope.callInProgress) {
                                $scope.inProgressCallFriendStartedTitleLoc = UtilFactory.getLocalizedStr($scope.inProgressCallFriendStartedTitleStr, CONTEXT_NAMESPACE, 'inprogresscallfriendstartedtitlestr', {
                                    '%username%': originId
                                });
                            }
                        } else {
                            VoiceFactory.setVoiceCallInfoForConversation(nucleusId, event, originId);
                            $scope.iscallingLoc = UtilFactory.getLocalizedStr($scope.iscallingStr, CONTEXT_NAMESPACE, 'iscallingstr', {
                                '%username%': originId
                            });
                            $scope.voiceCallState = event;
                            if(!$scope.callInProgress) {
                                $scope.inProgressCallFriendStartedTitleLoc = UtilFactory.getLocalizedStr($scope.inProgressCallFriendStartedTitleStr, CONTEXT_NAMESPACE, 'inprogresscallfriendstartedtitlestr', {
                                    '%username%': originId
                                });
                            }
                        }
                    }
                    else /* if (event === 'OUTGOING')*/ {
                        VoiceFactory.setVoiceCallInfoForConversation(nucleusId, event, originId);
                        $scope.callingLoc = UtilFactory.getLocalizedStr($scope.callingStr, CONTEXT_NAMESPACE, 'callingstr', {
                            '%username%': originId
                        });
                        $scope.voiceCallState = event;
                    }

                    // outgoing banner supercedes incoming banner
                    $scope.state.voiceCall = true;
                    break;

                default:
                    break;
            }
        }

        function handleVoiceCallEvent(voiceCallEventObj) {
            var nucleusId = $scope.conversation.participants[0];
            if (nucleusId === Number(voiceCallEventObj.nucleusId)) {
                updateVoiceState(nucleusId, originId, voiceCallEventObj.event);
                $scope.$digest();
            }
        }

        function extractSingleResponse(response) {
            return response[$scope.conversation.gameInvite.productId];
        }

        function assignCatalogInfoToScope(catalogInfo) {
            $scope.invitePackartSmall = catalogInfo.i18n.packArtSmall;
            $scope.inviteGameName = catalogInfo.i18n.displayName;
            $scope.$digest();
        }
        
        function handleGameInviteStateChanges() {
            if ((typeof $scope.presence !== 'undefined') && $scope.presence.gameActivity.joinable !== undefined) {
                // reset to default default states
                $scope.presence.isJoinable = false;
                $scope.state.invited = false;   
                
                if (typeof $scope.conversation !== 'undefined') {
                    $scope.presence.isJoinable = $scope.conversation.joinable;
                    $scope.state.invited = $scope.conversation.invited;

                    if ($scope.conversation.invited) {
                        // Get the game info
                        GamesDataFactory.getCatalogInfo([$scope.conversation.gameInvite.productId])
                            .then(extractSingleResponse)
                            .then(assignCatalogInfoToScope)
                            .catch(function(error) {
                                ComponentsLogFactory.error('[origin-social-chatwindow-conversation-banner] GamesDataFactory.getCatalogInfo', error);
                            });
                    }
                }
                $scope.$digest();
            }
        }

        function handleShowOutgoingInProgressCallBanner() {
            $scope.callInProgressOverride = true;
            $scope.state.voiceCall = true;
            $scope.voiceCallState = 'OUTGOING';
            $scope.inProgressCallUserStartedTitleLoc = UtilFactory.getLocalizedStr($scope.inProgressCallUserStartedTitleStr, CONTEXT_NAMESPACE, 'inprogresscalluserstartedntitlestr', {
                                '%username%': originId
                            });
            $scope.$digest();
        }

        $scope.hideOutgoingInProgressCallBanner = function() {
            $scope.callInProgressOverride = false;
            $scope.state.voiceCall = false;
            updateVoiceState(nucleusId, originId);
            ConversationFactory.inProgressCallBannerHidden(nucleusId);
            $scope.$digest();
        };

        function onDestroy() {
            var nucleusId = $scope.conversation.participants[0];
            ConversationFactory.events.off('gameInviteReceived:' + nucleusId, handleGameInviteStateChanges);
            ConversationFactory.events.off('gameInviteRevoked:' + nucleusId, handleGameInviteStateChanges);
            ConversationFactory.events.off('presenceChanged:' + nucleusId, handleGameInviteStateChanges);
            ConversationFactory.events.off('showOutgoingCallBanner:' + nucleusId, handleShowOutgoingInProgressCallBanner);
            ConversationFactory.events.off('hideInProgressCallBanner:' + nucleusId, $scope.hideOutgoingInProgressCallBanner);
            VoiceFactory.events.off('voiceCallEvent', handleVoiceCallEvent);
            VoiceFactory.voiceCallObservable().removeObserver(onVoiceCallChange);
        }

        function registerForEvents() {
            var nucleusId = $scope.conversation.participants[0];
            ConversationFactory.events.on('gameInviteReceived:' + nucleusId, handleGameInviteStateChanges);
            ConversationFactory.events.on('gameInviteRevoked:' + nucleusId, handleGameInviteStateChanges);
            ConversationFactory.events.on('presenceChanged:' + nucleusId, handleGameInviteStateChanges);
            ConversationFactory.events.on('showOutgoingCallBanner:' + nucleusId, handleShowOutgoingInProgressCallBanner);
            ConversationFactory.events.on('hideInProgressCallBanner:' + nucleusId, $scope.hideOutgoingInProgressCallBanner);
            VoiceFactory.events.on('voiceCallEvent', handleVoiceCallEvent);
            $scope.$on('$destroy', onDestroy);
        }

        if ($scope.conversation.state === 'ONE_ON_ONE') {
            registerForEvents();
            var nucleusId = $scope.conversation.participants[0];

            RosterDataFactory.getRosterUser(nucleusId).then(function(user) {
                if (!!user) {
                    ObserverFactory.create(user._presence)
                        .defaultTo({
                            jid: nucleusId,
                            presenceType: '',
                            show: 'UNAVAILABLE',
                            gameActivity: {}
                        })
                        .attachToScope($scope, 'presence');

                    // Get the friend name
                    originId = user.originId;
                    $scope.invitedToPlayLoc = UtilFactory.getLocalizedStr($scope.invitedToPlayStr, CONTEXT_NAMESPACE, 'invitedtoplaystr', {
                        '%username%': originId
                    });

                    updateVoiceState(nucleusId, user.originId, VoiceFactory.voiceCallInfoForConversation(nucleusId).state);
                    // Check for game invites when opening the conversation
                    handleGameInviteStateChanges();
                }
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
     * @param {LocalizedString} playingstr "Playing"playingStr
     * @param {LocalizedString} broadcastingstr "Broadcasting"
     * @param {LocalizedString} joingamecta "Join Game"
     * @param {LocalizedString} joingroupcta "Join Party"
     * @param {LocalizedString} joingameandgroupcta "Join Game and Party"
     * @param {LocalizedString} orstr "or"
     * @param {LocalizedString} watchstr "Watch"
     * @param {LocalizedString} ignorestr "Ignore"
     * @param {LocalizedString} callingstr "Calling %username%..."
     * @param {LocalizedString} cancelstr "Cancel"
     * @param {LocalizedString} incomingcallstr "Incoming call"
     * @param {LocalizedString} iscallingstr "%username% is calling"
     * @param {LocalizedString} invitedtoplaystr "%username% invited you to play:"
     * @param {LocalizedString} playingingroupstr " in a Party."
     * @param {LocalizedString} playingingroupwithnamestr " in Party %group%."
     * @param {LocalizedString} inprogresscalluserstartedntitlestr "Are you sure you want to call %username%?"
     * @param {LocalizedString} inprogresscalluserstartedndescriptionstr "Starting this call will end your current voice chat."
     * @param {LocalizedString} inprogresscallfriendstartedtitlestr "%username% is calling"
     * @param {LocalizedString} inprogresscallfriendstarteddescriptionstr "Answering this will end your current voice chat."
     * @param {LocalizedString} answercallstr "answer call"
     * @param {LocalizedString} ignorecallstr "ignore call"
     * @param {LocalizedString} join-game-close-button-text *Update in defaults
     * @param {LocalizedString} join-game-download-button-text *Update in defaults
     * @param {LocalizedString} join-game-not-installed-text *Update in defaults
     * @param {LocalizedString} join-game-not-owned-text *Update in defaults
     * @param {LocalizedString} join-game-unable-text *Update in defaults
     * @param {LocalizedString} join-game-view-in-store-button-text *Update in defaults
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
     *            ignorestr="Ignore"
     *            callingstr="Calling %username%..."
     *            cancelstr="Cancel"
     *            incomingcallstr="Incoming call"
     *            iscallingstr="%username% is calling"
     *            invitedtoplaystr="%username% invited you to play:"
     *            inprogresscalluserstartedntitlestr="Are you sure you want to call %username%?"
     *            inprogresscalluserstartedndescriptionstr="Starting this call will end your current voice chat."
     *            inprogresscallfriendstartedtitlestr="%username% is calling"
     *            inprogresscallfriendstarteddescriptionstr="Answering this will end your current voice chat."
     *            answercallstr="answer call"
     *            ignorecallstr="ignore call"
     *         ></origin-social-chatwindow-conversation-banner>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialChatwindowConversationBannerCtrl', OriginSocialChatwindowConversationBannerCtrl)
        .directive('originSocialChatwindowConversationBanner', function(ComponentsConfigFactory, NavigationFactory, JoinGameFactory, ConversationFactory, VoiceFactory, ProductNavigationFactory) {

            return {
                restrict: 'E',
                controller: 'OriginSocialChatwindowConversationBannerCtrl',
                scope: {
                    conversation: '=',
                    playingStr: '@playingstr',
                    broadcastingStr: '@broadcastingstr',
                    joinGameLoc: '@joingamecta',
                    joinGroupLoc: '@joingroupcta',
                    joinGameAndGroupLoc: '@joingameandgroupcta',
                    orStr: '@orstr',
                    watchStr: '@watchstr',
                    ignoreStr: '@ignorestr',
                    callingStr: '@callingstr',
                    cancelStr: '@cancelstr',
                    incomingcallStr: '@incomingcallstr',
                    iscallingStr: '@iscallingstr',
                    invitedToPlayStr: '@invitedtoplaystr',
                    playingInGroupLoc: '@playingingroupstr',
                    playingInGroupWithNameLoc: '@playingingroupwithnamestr',
                    inProgressCallUserStartedTitleStr: '@inprogresscalluserstartedntitlestr',
                    inProgressCallUserStartedDescriptionStr: '@inprogresscalluserstartedndescriptionstr',
                    inProgressCallFriendStartedTitleStr: '@inprogresscallfriendstartedtitlestr',
                    inProgressCallFriendStartedDescriptionStr: '@inprogresscallfriendstarteddescriptionstr',
                    answerCallStr: '@answercallstr',
                    ignoreCallStr: '@ignorecallstr',
                    joinGameCloseButtonText: '@',
                    joinGameDownloadButtonText: '@',
                    joinGameNotInstalledText: '@',
                    joinGameNotOwnedText: '@',
                    joinGameUnableText: '@',
                    joinGameViewInStoreButtonText: '@'
                },
                link: function(scope, element) {

                    // Clicked on a product link
                    $(element).on('click', '.origin-social-chatwindow-conversation-banner-productlink', function(event) {
                        var productId = $(event.target).attr('data-product-id');
                        var multiplayerId = $(event.target).attr('data-multiplayer-id');
                        if (!!productId && !!multiplayerId) {
                            ProductNavigationFactory.goProductDetailsWithMultiplayerId(productId, multiplayerId);
                        } else if (!!productId) {
                            ProductNavigationFactory.goProductDetails(productId);
                        }
                    });

                    // Clicked "Join"
                    $(element).on('click', '.origin-social-chatwindow-conversation-banner-join', function() {

                        var nucleusId = scope.conversation.participants[0];
                        var productId = scope.presence.gameActivity.productId;
                        var multiplayerId = scope.presence.gameActivity.multiplayerId;
                        var gameTitle = scope.presence.gameActivity.gameTitle;
                        JoinGameFactory.joinFriendsGame(nucleusId, productId, multiplayerId, gameTitle, scope.joinGameStrings);

                    });

                    // Clicked "Watch"
                    $(element).on('click', '.origin-social-chatwindow-conversation-banner-watch', function(event) {
                        var url = $(event.target).attr('data-broadcast-url');
                        if (!!url) {
                            NavigationFactory.asyncOpenUrl(url);
                        }
                    });

                    // Invite clicked
                    function ignoreInvitation(nucleusId) {
                        ConversationFactory.ignoreInvitation(nucleusId);
                    }

                    // Join Game clicked
                    function acceptInvitation(nucleusId) {
                        ConversationFactory.acceptInvitation(nucleusId);
                    } 

                    function onDestroy() {
                        $(element).off('click');
                        angular.element(element).off('click');
                    }

                    scope.$on('$destroy', onDestroy);

                    // Clicked Game Invite "Join"
                    $(element).on('click', '.origin-social-chatwindow-conversation-banner-gameinvite-join', function() {
                        // Join the game
                        var nucleusId = scope.conversation.participants[0];
                        var productId = scope.conversation.gameInvite.productId;
                        var multiplayerId = scope.conversation.gameInvite.multiplayerId;
                        var gameTitle = scope.inviteGameName;
                        JoinGameFactory.joinFriendsGame(nucleusId, productId, multiplayerId, gameTitle, scope.joinGameStrings);
                        
                        acceptInvitation(nucleusId);

                    });

                    // Clicked Game Invite "Ignore"
                    $(element).on('click', '.origin-social-chatwindow-conversation-banner-gameinvite-ignore', function() {
                         var nucleusId = scope.conversation.participants[0];
                         ignoreInvitation(nucleusId);
                    });

                    // Clicked Game Invite Game Name
                    $(element).on('click', '.origin-social-chatwindow-conversation-banner-gameinvite-gamename', function() {
                        ProductNavigationFactory.goProductDetailsWithMultiplayerId(scope.gameInvite.productId, scope.gameInvite.multiplayerId);
                    });

                    // Clicked Game Invite box art
                    $(element).on('click', '.origin-social-chatwindow-conversation-banner-packart', function() {
                        ProductNavigationFactory.goProductDetailsWithMultiplayerId(scope.gameInvite.productId, scope.gameInvite.multiplayerId);
                    });

                    // Clicked 'Cancel' on a voice call
                    angular.element(element).on('click', '.origin-social-chatwindow-conversation-banner-voicecall-cancel', function() {
                        Origin.voice.leaveVoice(scope.conversation.id);
                    });

                    // Clicked 'Answer' on a voice call
                    angular.element(element).on('click', '.origin-social-chatwindow-conversation-banner-voicecall-action-link-answer', function() {
                        Origin.voice.answerCall(scope.conversation.id);
                    });

                    // Clicked 'Ignore' on a voice call
                    angular.element(element).on('click', '.origin-social-chatwindow-conversation-banner-voicecall-action-link-ignore', function() {
                        Origin.voice.ignoreCall(scope.conversation.id);
                    });

                    // Clicked 'Accept' on a outgoing voice call with one already in progress
                    angular.element(element).on('click', '.origin-social-chatwindow-conversation-banner-inprogressvoicecall-action-link-accept', function() {
                        var newCallID = scope.conversation.participants[0];
                        var oldCallID = VoiceFactory.activeVoiceConversationId();
                        if(oldCallID) {
                            Origin.voice.leaveVoice(oldCallID);
                        }
                        if(newCallID) {
                            Origin.voice.joinVoice(newCallID, scope.conversation.participants);
                            scope.hideOutgoingInProgressCallBanner();
                        }
                    });

                    // Clicked 'End' on a outgoing voice call with one already in progress
                    angular.element(element).on('click', '.origin-social-chatwindow-conversation-banner-inprogressvoicecall-action-link-cancel', function() {
                        scope.hideOutgoingInProgressCallBanner();
                    });
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/chatwindow/conversation/views/banner.html')
            };

        });
}());
