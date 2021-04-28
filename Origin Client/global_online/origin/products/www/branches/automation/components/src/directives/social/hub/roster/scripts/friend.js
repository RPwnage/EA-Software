/**
 * Created by tirhodes on 2015-02-23.
 * @file friend.js
 */
(function () {

    'use strict';
    var panelHeight = 0;

    /**
    * The controller
    */
    function OriginSocialHubRosterFriendCtrl($scope, $window, $timeout, $sce, RosterDataFactory, UserDataFactory, ConversationFactory, ComponentsConfigFactory, DialogFactory, UtilFactory, NavigationFactory, EventsHelperFactory, AuthFactory, ComponentsLogFactory, JoinGameFactory, ObserverFactory, VoiceFactory) {
        $scope.contextmenuCallbacks = {};
        $scope.isVoiceSupported = Origin.voice.supported();
        $scope.isContextmenuVisible = false;
        $scope.playingJoinableGame = false;

        var self = this;
        var $element = null;
        var CONTEXT_NAMESPACE = 'origin-social-hub-roster-friend';

        var joinGameStrings = {
            unableToJoinText: UtilFactory.getLocalizedStr($scope.joinGameUnableText, CONTEXT_NAMESPACE, 'join-game-unable-text'),
            notOwnedText: UtilFactory.getLocalizedStr($scope.joinGameNotOwnedText, CONTEXT_NAMESPACE, 'join-game-not-owned-text'),
            viewInStoreText: UtilFactory.getLocalizedStr($scope.joinGameViewInStoreButtonText, CONTEXT_NAMESPACE, 'join-game-view-in-store-button-text'),
            closeText: UtilFactory.getLocalizedStr($scope.joinGameCloseButtonText, CONTEXT_NAMESPACE, 'join-game-close-button-text'),
            notInstalledText: UtilFactory.getLocalizedStr($scope.joinGameNotInstalledText, CONTEXT_NAMESPACE, 'join-game-not-installed-text'),
            downloadText: UtilFactory.getLocalizedStr($scope.joinGameDownloadButtonText, CONTEXT_NAMESPACE, 'join-game-download-button-text')
        };

        $scope.onlineLoc = UtilFactory.getLocalizedStr($scope.onlineStr, CONTEXT_NAMESPACE, 'onlinestr');
        $scope.awayLoc = UtilFactory.getLocalizedStr($scope.awayStr, CONTEXT_NAMESPACE, 'awaystr');
        $scope.offlineLoc = UtilFactory.getLocalizedStr($scope.offlineStr, CONTEXT_NAMESPACE, 'offlinestr');
        $scope.broadcastingLoc = UtilFactory.getLocalizedStr($scope.broadcasting, CONTEXT_NAMESPACE, 'broadcastingstr');
        $scope.joinGameLoc = UtilFactory.getLocalizedStr($scope.joinGameLoc, CONTEXT_NAMESPACE, 'joingamecta');
        $scope.joinGroupLoc = UtilFactory.getLocalizedStr($scope.joinGroupLoc, CONTEXT_NAMESPACE, 'joingroupcta');
        $scope.joinGameAndGroupLoc = UtilFactory.getLocalizedStr($scope.joinGameAndGroupLoc, CONTEXT_NAMESPACE, 'joingameandgroupcta');
        $scope.playingInGroupLoc = UtilFactory.getLocalizedStr($scope.playingInGroupLoc, CONTEXT_NAMESPACE, 'playingingroup');
        $scope.playingInGroupWithNameLoc = UtilFactory.getLocalizedStr($scope.playingInGroupWithNameLoc, CONTEXT_NAMESPACE, 'playingingroupwithname');
        $scope.sendMessageLoc = UtilFactory.getLocalizedStr($scope.sendMessageLoc, CONTEXT_NAMESPACE, 'sendmessagestr');
        $scope.startVoiceChatLoc = UtilFactory.getLocalizedStr($scope.startVoiceChatLoc, CONTEXT_NAMESPACE, 'startvoicechatstr');
        $scope.viewProfileLoc = UtilFactory.getLocalizedStr($scope.viewProfileLoc, CONTEXT_NAMESPACE, 'viewprofilestr');
        $scope.watchBroadcastLoc = UtilFactory.getLocalizedStr($scope.watchBroadcastLoc, CONTEXT_NAMESPACE, 'watchbroadcaststr');
        $scope.inviteToGameLoc = UtilFactory.getLocalizedStr($scope.inviteToGameLoc, CONTEXT_NAMESPACE, 'invitetogamestr');
        $scope.unfriendAndBlockLoc = UtilFactory.getLocalizedStr($scope.unfriendAndBlockLoc, CONTEXT_NAMESPACE, 'unfriendandblockstr');
        $scope.unfriendLoc = UtilFactory.getLocalizedStr($scope.unfriendLoc, CONTEXT_NAMESPACE, 'unfriendstr');

        var callThrottledScopeDigest = UtilFactory.throttle(function() {
            $timeout(function() { $scope.$digest(); }, 0, false);
        }, 2000);

        var handles = [];

        RosterDataFactory.getRosterUser($scope.friend.nucleusId).then(function(user) {
            if (!!user) {
                ObserverFactory.create(user._presence)
                    .defaultTo({jid: $scope.friend.nucleusId, presenceType: '', show: 'UNAVAILABLE', gameActivity: {}})
                    .attachToScope($scope, 'presence');
            }
        });

        RosterDataFactory.getRosterUser(Origin.user.userPid()).then(function(user) {
            if (!!user) {
                ObserverFactory.create(user._presence)
                    .defaultTo({jid: Origin.user.userPid(), presenceType: '', show: 'UNAVAILABLE', gameActivity: {}})
                    .attachToScope($scope, 'selfPresence');
            }
        });

        function isCurrentConversationInVoip() {
            var activeVoiceConversationId = VoiceFactory.activeVoiceConversationId();
            return VoiceFactory.voiceCallObservable().data.isInVoice && (activeVoiceConversationId === $scope.friend.nucleusId);
        }

        function handleVoiceCallUpdate() {
                $scope.inVoip = isCurrentConversationInVoip();
                $scope.calling = VoiceFactory.voiceCallObservable().data.isCalling;
                $scope.isLocalMuted = VoiceFactory.voiceCallObservable().data.isLocalMuted;
            }
        $scope.inVoip = isCurrentConversationInVoip();

        VoiceFactory.voiceCallObservable().addObserver(handleVoiceCallUpdate);

        this.setElement = function(element) {
            $element = element;
        };

        function isThisFriendVisible() {
            var visible = $scope.sectionController.isFriendVisible($scope.index, panelHeight);
            //console.log('TR: isThisFriendVisible: index: ' + $scope.index + ' ' + $scope.friend.originId + ' ' + visible);
            return visible;
        }

        function requestAvatar() {
            UserDataFactory.getAvatar($scope.friend.nucleusId, Origin.defines.avatarSizes.SMALL)
                .then(function(response) {

                    $scope.friend.avatarImgSrc = response;
                    callThrottledScopeDigest();
                }, function() {

                }).catch(function(error) {

                    ComponentsLogFactory.error('OriginSocialHubRosterFriendCtrl: UserDataFactory.getAvatar failed', error);
                });
        }

        function handleUpdate() {
            //console.log('TR: handleUpdate: ' + $scope.friend.originId);

            if (isThisFriendVisible()) {
                //console.log('TR: handleUpdate: visible: ' + $scope.friend.originId);

                if ((typeof $scope.friend.avatarImgSrc === 'undefined') || ($scope.friend.avatarImgSrc === ComponentsConfigFactory.getImagePath('social//avatar_placeholder.png'))) {
                    requestAvatar();
                }

                if ( (typeof $scope.friend.lastName !== 'undefined') && ($scope.friend.lastName !== null) && ($scope.friend.lastName.length === 0) &&
                    (typeof $scope.friend.firstName !== 'undefined') && ($scope.friend.firstName !== null) && ($scope.friend.firstName.length === 0) ) {
                    UserDataFactory.getUserRealName($scope.friend.nucleusId);
                }

            }
        }

        handles[handles.length] = RosterDataFactory.events.on('updateFriendsDirective' + $scope.friend.nucleusId, handleUpdate);

        function handleNameUpdate(friend) {
            $scope.friend.firstName = (friend.firstName ? friend.firstName : $scope.friend.firstName);
            $scope.friend.lastName = (friend.lastName ? friend.lastName : $scope.friend.lastName);
            $scope.$digest();
        }

        handles[handles.length] = UserDataFactory.events.on('socialFriendsFullNameUpdated:' + $scope.friend.nucleusId, handleNameUpdate);

        this.openConversation = function() {
            ConversationFactory.openConversation($scope.friend.nucleusId);
        };

        // set up panel height for limit to calculations
        this.setPanelHeight = function() {
            panelHeight = $element.outerHeight();
            RosterDataFactory.setFriendPanelHeight(panelHeight);
        };

        // call once initially
        this.initialUpdate = function() {
            handleUpdate();
        };

        if ($scope.sectionController) {
            handleUpdate();
        }

        $scope.trustAsHtml = function (html) {
            return $sce.trustAsHtml(html);
        };

        /* Contextmenu event callbacks
        * --------------------------
        */
        $scope.contextmenuCallbacks.sendMessage = function () {
            ConversationFactory.openConversation($scope.friend.nucleusId);
            $scope.isContextmenuVisible = false;
        };
        $scope.contextmenuCallbacks.inviteToChatroom = function (chatroom) {
            $window.alert('This is the "invite to chatroom" button. I invite you to ' + chatroom.title + ' chatoomroom. Make haste!');
            $scope.isContextmenuVisible = false;
        };
        $scope.contextmenuCallbacks.startVoiceChat = function () {
            $scope.inVoip = isCurrentConversationInVoip();
            if($scope.friend.nucleusId) {
                if(!$scope.inVoip && VoiceFactory.activeVoiceConversationId() === 0){
                    // not yet in a call
                    ConversationFactory.openConversation($scope.friend.nucleusId);
                    Origin.voice.joinVoice($scope.friend.nucleusId, [$scope.friend.nucleusId]);
                    VoiceFactory.setActiveVoiceConversationId($scope.friend.nucleusId);
                }
                else {
                    // in a call - show incall dialog
                    ConversationFactory.showInProgressCallBanner($scope.friend.nucleusId);
                }
            }
            $scope.isContextmenuVisible = false;
        };
        $scope.contextmenuCallbacks.viewProfile = function () {
            if (Origin.client.oig.IGOIsActive()) {
                // Open OIG profile window
                NavigationFactory.openIGOSPA('PROFILE', $scope.friend.nucleusId);
            }
            else {
                NavigationFactory.goUserProfile($scope.friend.nucleusId);
            }
            $scope.isContextmenuVisible = false;
        };
        $scope.contextmenuCallbacks.joinGame = function () {

            var nucleusId = $scope.friend.nucleusId;
            var productId = $scope.friend.presence.gameActivity.productId;
            var multiplayerId = $scope.friend.presence.gameActivity.multiplayerId;
            var gameTitle = $scope.friend.presence.gameActivity.title;
            JoinGameFactory.joinFriendsGame(nucleusId, productId, multiplayerId, gameTitle, joinGameStrings);

            $scope.isContextmenuVisible = false;
        };
        $scope.contextmenuCallbacks.watchBroadcast = function () {
            var url = $scope.friend.presence.gameActivity.twitchPresence;
            if (!!url) {
                NavigationFactory.asyncOpenUrl(url);
            }
            $scope.isContextmenuVisible = false;
        };
        $scope.contextmenuCallbacks.inviteToGame = function () {
            Origin.xmpp.inviteToGame($scope.friend.nucleusId);
            $scope.isContextmenuVisible = false;
        };


        $scope.removeFriendAndBlockDlgStrings = {};
        $scope.removeFriendAndBlockDlgStrings.title = UtilFactory.getLocalizedStr($scope.unfriendandblocktitle, CONTEXT_NAMESPACE, 'unfriendandblocktitle');
        $scope.removeFriendAndBlockDlgStrings.description = UtilFactory.getLocalizedStr($scope.unfriendandblockdescription, CONTEXT_NAMESPACE, 'unfriendandblockdescription');
        $scope.removeFriendAndBlockDlgStrings.description2 = UtilFactory.getLocalizedStr($scope.unfriendandblockdescription2, CONTEXT_NAMESPACE, 'unfriendandblockdescription_2');
        $scope.removeFriendAndBlockDlgStrings.description3 = UtilFactory.getLocalizedStr($scope.unfriendandblockdescription3, CONTEXT_NAMESPACE, 'unfriendandblockdescription_3');
        $scope.removeFriendAndBlockDlgStrings.acceptText = UtilFactory.getLocalizedStr($scope.unfriendandblockaccept, CONTEXT_NAMESPACE, 'unfriendandblockaccept');
        $scope.removeFriendAndBlockDlgStrings.rejectText = UtilFactory.getLocalizedStr($scope.unfriendandblockreject, CONTEXT_NAMESPACE, 'unfriendandblockreject');

        this.handleRemoveFriendAndBlock = function(result) {
            if (result.accepted) {
                RosterDataFactory.removeFriendAndBlock($scope.friend.nucleusId);
            }
        };
        $scope.contextmenuCallbacks.removeFriendAndBlock = function () {
            $scope.rosterContextmenuVisible = false;

            var description = '<b>' + $scope.removeFriendAndBlockDlgStrings.description + '</b>' + '<br><br>' +
                $scope.removeFriendAndBlockDlgStrings.description2 + '<br><br>' +
                $scope.removeFriendAndBlockDlgStrings.description3;
            description = description.replace(/%username%/g, $scope.friend.originId);

            DialogFactory.openPrompt({
                id: 'social-hub-roster-removeFriendAndBlock-id' + $scope.friend.nucleusId,
                name: 'social-hub-roster-removeFriendAndBlock',
                title: $scope.removeFriendAndBlockDlgStrings.title,
                description: description,
                acceptText: $scope.removeFriendAndBlockDlgStrings.acceptText,
                acceptEnabled: true,
                rejectText: $scope.removeFriendAndBlockDlgStrings.rejectText,
                defaultBtn: 'yes',
                callback: self.handleRemoveFriendAndBlock
            });

            $scope.isContextmenuVisible = false;

        };

        this.handleRemoveFriend = function(result) {
            if (result.accepted) {
                RosterDataFactory.removeFriend($scope.friend.nucleusId);
            }
        };

        $scope.removeFriendDlgStrings = {};
        $scope.removeFriendDlgStrings.title = UtilFactory.getLocalizedStr($scope.unfriendusertitle, CONTEXT_NAMESPACE, 'unfriendusertitle');
        $scope.removeFriendDlgStrings.description = UtilFactory.getLocalizedStr($scope.unfrienduserdescription, CONTEXT_NAMESPACE, 'unfrienduserdescription');
        $scope.removeFriendDlgStrings.acceptText = UtilFactory.getLocalizedStr($scope.unfrienduseraccept, CONTEXT_NAMESPACE, 'unfrienduseraccept');
        $scope.removeFriendDlgStrings.rejectText = UtilFactory.getLocalizedStr($scope.unfrienduserreject, CONTEXT_NAMESPACE, 'unfrienduserreject');

        $scope.contextmenuCallbacks.removeFriend = function () {
            DialogFactory.openPrompt({
                id: 'social-hub-roster-removeFriend-id-' + $scope.friend.nucleusId,
                name: 'social-hub-roster-removeFriend',
                title: $scope.removeFriendDlgStrings.title,
                description: $scope.removeFriendDlgStrings.description.replace('%username%', $scope.friend.originId),
                acceptText: $scope.removeFriendDlgStrings.acceptText,
                acceptEnabled: true,
                rejectText: $scope.removeFriendDlgStrings.rejectText,
                defaultBtn: 'yes',
                callback: self.handleRemoveFriend
            });
        };

        function onClientOnlineStateChanged(onlineObj) {
            if (!!onlineObj && !onlineObj.isOnline) {
                DialogFactory.close('social-hub-roster-removeFriend-id');
                DialogFactory.close('social-hub-roster-removeFriendAndBlock-id');
            }
        }

        handles[handles.length] = AuthFactory.events.on('myauth:clientonlinestatechangeinitiated', onClientOnlineStateChanged);

        function onDestroy() {
            EventsHelperFactory.detachAll(handles);
            VoiceFactory.voiceCallObservable().removeObserver(handleVoiceCallUpdate);
        }

        $scope.$on('$destroy', onDestroy);
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialHubRosterFriend
     * @restrict E
     * @element ANY
     * @scope
     * @param {Object} friend Javascript object representing the friend for this friend item
     * @param {integer} index index of the friend item in the viewport
     * @param {LocalizedString} unfriendusertitle title of the Unfriend User dialog
     * @param {LocalizedString} unfrienduserdescription Description of the Unfriend User dialog
     * @param {LocalizedString} unfrienduseraccept Accept prompt of the Unfriend User dialog
     * @param {LocalizedString} unfrienduserreject Reject prompt of the Unfriend User dialog
     * @param {LocalizedString} unfriendandblocktitle title of the Unfriend And Block dialog
     * @param {LocalizedString} unfriendandblockdescription Description of the Unfriend And Block dialog
     * @param {LocalizedString} unfriendandblockdescription_2 Description of the Unfriend And Block dialog
     * @param {LocalizedString} unfriendandblockdescription_3 Description of the Unfriend And Block dialog
     * @param {LocalizedString} unfriendandblockaccept Accept prompt of the Unfriend And Block dialog
     * @param {LocalizedString} unfriendandblockreject Reject prompt of the Unfriend And Block dialog
     * @param {LocalizedString} onlinestr "Online" label
     * @param {LocalizedString} awaystr "Away" label
     * @param {LocalizedString} offlinestr "Offline" label
     * @param {LocalizedString} joingamecta *A String that does nothing use defaults to change
     * @param {LocalizedString} broadcastingstr "Watch Broadcast" tooltip
     * @param {LocalizedString} joingroupcta "Join Party" tooltip
     * @param {LocalizedString} joingameandgroupcta "Join Game and Party" tooltip
     * @param {LocalizedString} playingingroup "Playing in a group."
     * @param {LocalizedString} playingingroupwithname "Playing in group: %group%."
     * @param {LocalizedString} sendmessagestr Send Message
     * @param {LocalizedString} startvoicechatstr Start Voice Chat
     * @param {LocalizedString} viewprofilestr View Profile
     * @param {LocalizedString} watchbroadcaststr Watch Broadcast
     * @param {LocalizedString} invitetogamestr Invite to Game
     * @param {LocalizedString} unfriendandblockstr Unfriend and Block
     * @param {LocalizedString} unfriendstr Unfriend
     * @param {LocalizedString} join-game-close-button-text *A String that does nothing use defaults to change
     * @param {LocalizedString} join-game-download-button-text *A String that does nothing use defaults to change
     * @param {LocalizedString} join-game-unable-text *A String that does nothing use defaults to change
     * @param {LocalizedString} join-game-not-owned-text *A String that does nothing use defaults to change
     * @param {LocalizedString} join-game-view-in-store-button-text *A String that does nothing use defaults to change
     * @param {LocalizedString} join-game-not-installed-text *A String that does nothing use defaults to change
     *
     *
     * @description
     *
     * origin social hub -> roster area -> friend
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-social-hub-roster-friend
     *            friend="friend"
     *            index=index
	 *			  unfrienduserdescription="Are you sure you want to unfriend %username%?"
	 *			  unfriendusertitle="Unfriend user"
	 *			  unfrienduseraccept="Yes"
	 *			  unfrienduserreject="No"
	 *			  unfriendandblockdescription="Are you sure you want to unfriend and block %username%?"
	 *			  unfriendandblocktitle="Unfriend And Block"
	 *			  unfriendandblockaccept="Yes"
	 *			  unfriendandblockreject="No"
	 *			  onlinestr="Online"
	 *            offlinestr="Offline"
	 *            awaystr="Away"
	 *            broadcastingstr="Watch Broadcast"
     *         ></origin-social-hub-roster-friend>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialHubRosterFriendCtrl', OriginSocialHubRosterFriendCtrl)
        .directive('originSocialHubRosterFriend', function (ComponentsConfigFactory, RosterDataFactory, KeyEventsHelper) {

            return {
                restrict: 'E',
                controller: 'OriginSocialHubRosterFriendCtrl',
                require: ['^originSocialHubRosterSection', 'originSocialHubRosterFriend'],
                scope: {
                    'friend': '=',
                    'index': '=',
                    'unfriendusertitle': '@',
                    'unfrienduserdescription': '@',
                    'unfrienduseraccept': '@',
                    'unfrienduserreject': '@',
                    'unfriendandblocktitle': '@',
                    'unfriendandblockdescription': '@',
                    'unfriendandblockdescription_2': '@',
                    'unfriendandblockdescription_3': '@',
                    'unfriendandblockaccept': '@',
                    'unfriendandblockreject': '@',
                    'onlineStr': '@onlinestr',
                    'offlineStr': '@offlinestr',
                    'awayStr': '@awaystr',
                    'broadcasting': '@broadcastingstr',
                    'joinGameLoc': '@joingamecta',
                    'joinGroupLoc': '@joingroupcta',
                    'joinGameAndGroupLoc': '@joingameandgroupcta',
                    'playingInGroupLoc': '@playingingroup',
                    'playingInGroupWithNameLoc': '@playingingroupwithname',
                    'sendMessageLoc': '@sendmessagestr',
                    'startVoiceChatLoc': '@startvoicechatstr',
                    'viewProfileLoc': '@viewprofilestr',
                    'watchBroadcastLoc': '@watchbroadcaststr',
                    'inviteToGameLoc': '@invitetogamestr',
                    'unfriendAndBlockLoc': '@unfriendandblockstr',
                    'unfriendLoc': '@unfriendstr',
                    'joinGameUnableText': '@',
                    'joinGameNotOwnedText': '@',
                    'joinGameViewInStoreButtonText': '@',
                    'joinGameCloseButtonText': '@',
                    'joinGameNotInstalledText': '@',
                    'joinGameDownloadButtonText': '@'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/hub/roster/views/friend.html'),
                link: function(scope, elem, attrs, ctrl) {
                    attrs = attrs;

                    scope.sectionController = ctrl[0];
                    var thisController = ctrl[1];

                    if (panelHeight === 0) {
                        thisController.setElement(angular.element(elem).find('.origin-social-hub-roster-friend'));
                        thisController.setPanelHeight();
                    }

                    function handleDblClick() {
                        thisController.openConversation();
                    }
                    function handleLeftClick(e) {
                        // There's an icon to icon-sider
                        if (angular.element(e.target).hasClass('origin-social-hub-roster-contextmenu-otkicon-downarrow') && !scope.isContextmenuVisible) {
                            openContextmenu();
                        } else {
                            closeContextmenu();
                            angular.element(elem).focus();
                        }
                    }
                    function handleRightClick(e) {
                        e.preventDefault();
                        openContextmenu();
                        angular.element(elem).focus();
                    }
                    function handleRightKeyup(e) {
                        e.stopPropagation();
                        openContextmenu();
                    }
                    function handleLeftKeyup(e) {
                        e.stopPropagation();
                        closeContextmenu();
                        angular.element(elem).focus();
                    }

                    function handleEnter() {
                        //if we hit enter on the friend item, trigger the double click to open the conversation
                        var clickElement = angular.element(document.activeElement).find('[keytarget]');
                        if (clickElement) {
                            handleDblClick();
                            return false;
                        }
                    }
                    function openContextmenu() {

                        // Figure out what direction in which to display the context menu - up or down
                        var viewportHeight = RosterDataFactory.getRosterViewportHeight();
                        var friendYPos = scope.sectionController.friendYPosition(scope.index, RosterDataFactory.getFriendPanelHeight());
                        scope.displayContextmenuUp = friendYPos > (viewportHeight / 2) ? true : false;

                        scope.isContextmenuVisible = true;
                        scope.$digest();
                    }

                    function closeContextmenu() {
                        if (scope.isContextmenuVisible) {
                            scope.isContextmenuVisible = false;
                            scope.$digest();
                        }
                    }

                    function handleInitiateVoiceShortcut() {
                        scope.contextmenuCallbacks.startVoiceChat();
                    }

                    function handleKeyup(e) {
                        var altActive = event.altKey,
                            shiftActive = event.shiftKey;

                        if (shiftActive && altActive) {
                            return KeyEventsHelper.mapEvents(event, {
                                'enter': handleInitiateVoiceShortcut
                            });
                        } else {
                            return KeyEventsHelper.mapEvents(e, {
                                'right': handleRightKeyup,
                                'left': handleLeftKeyup,
                                'enter': handleEnter
                            });
                        }
                    }

                    function handleBroadcastIconClick() {
                        scope.contextmenuCallbacks.watchBroadcast();
                    }

                    function handleJoingameIconClick() {
                        scope.contextmenuCallbacks.joinGame();
                    }

                    function handleFocusOut() {
                        //run this on the next tick to give the active element a chance to set
                        setTimeout(function() {
                            if (!angular.element(elem).has(document.activeElement).length) {
                                closeContextmenu();
                            }
                        }, 0);
                    }

                    angular.element(elem).on('dblclick', handleDblClick);
                    angular.element(elem).on('click', handleLeftClick);
                    angular.element(elem).bind('contextmenu', handleRightClick);
                    angular.element(elem).on('keyup', handleKeyup);
                    angular.element(elem).on('click', '.origin-social-hub-roster-friend-watchbroadcast-icon', handleBroadcastIconClick);
                    angular.element(elem).on('click', '.origin-social-hub-roster-friend-joingame-icon', handleJoingameIconClick);
                    angular.element(elem).on('focusout', handleFocusOut);

                    function onDestroy () {
                        angular.element(elem).off('dblclick', handleDblClick);
                        angular.element(elem).off('click', handleLeftClick);
                        angular.element(elem).unbind('contextmenu', handleRightClick);
                        angular.element(elem).off('keyup', handleKeyup);
                        angular.element(elem).off('click', '.origin-social-hub-roster-friend-watchbroadcast-icon', handleBroadcastIconClick);
                        angular.element(elem).off('click', '.origin-social-hub-roster-friend-joingame-icon', handleJoingameIconClick);
                        angular.element(elem).off('focusout', handleFocusOut);

                    }
                    scope.$on('$destroy', onDestroy);

                    // Set up placeholder avatar
                    if (typeof scope.friend.avatarImgSrc === 'undefined') {
                        scope.friend.avatarImgSrc = ComponentsConfigFactory.getImagePath('social//avatar_placeholder.png');
                    }

                    thisController.initialUpdate();

                }
            };

        });

}());
