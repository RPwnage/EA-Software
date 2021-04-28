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
    function OriginSocialHubRosterFriendCtrl($scope, $window, $timeout, RosterDataFactory, UserDataFactory, ConversationFactory, ComponentsConfigFactory,
                                             DialogFactory, AppCommFactory, UtilFactory, NavigationFactory, EventsHelperFactory, AuthFactory, ComponentsLogFactory, JoinGameFactory) {
        $scope.contextmenuCallbacks = {};
        $scope.isVoiceSupported = Origin.voice.supported();
        $scope.isContextmenuVisible = false;
        $scope.playingJoinableGame = false;

		var self = this;
        var $element = null;
        var CONTEXT_NAMESPACE = 'origin-social-hub-roster-friend';

		$scope.onlineLoc = UtilFactory.getLocalizedStr($scope.onlineStr, CONTEXT_NAMESPACE, 'onlinestr');
		$scope.awayLoc = UtilFactory.getLocalizedStr($scope.awayStr, CONTEXT_NAMESPACE, 'awaystr');
		$scope.offlineLoc = UtilFactory.getLocalizedStr($scope.offlineStr, CONTEXT_NAMESPACE, 'offlinestr');
        $scope.broadcastingLoc = UtilFactory.getLocalizedStr($scope.broadcasting, CONTEXT_NAMESPACE, 'broadcastingstr');
        $scope.joinGameLoc = UtilFactory.getLocalizedStr($scope.joingame, CONTEXT_NAMESPACE, 'joingamestr');

		 var callThrottledScopeDigest = UtilFactory.throttle(function() {
			 $timeout(function() { $scope.$digest(); }, 0, false);
		 }, 2000);

        var handles = [];


        // set up window handler so the events are disconnected upon the window closing
        function destroy() {
            EventsHelperFactory.detachAll(handles);
        }
        window.addEventListener('unload', destroy);

        this.setElement = function(element) {
            $element = element;
        };

        function isThisFriendVisible() {
            var visible = $scope.sectionController.isFriendVisible($scope.index, panelHeight);
			//console.log('TR: isThisFriendVisible: index: ' + $scope.index + ' ' + $scope.friend.originId + ' ' + visible);
            return visible;
        }

		function requestAvatar() {
			UserDataFactory.getAvatar($scope.friend.nucleusId, 'AVATAR_SZ_SMALL')
				.then(function(response) {

					$scope.friend.avatarImgSrc = response;
					callThrottledScopeDigest();
				}, function() {

				}).catch(function(error) {

					ComponentsLogFactory.error('OriginSocialHubRosterFriendCtrl: UserDataFactory.getAvatar failed', error);
				});
		}

        function handlePresenceChange() {
            RosterDataFactory.getFriendSocialInfo($scope.friend.nucleusId).then(function(socialPresence) {
                if (!!socialPresence.presence) {

					if ($scope.friend.presence !== socialPresence.presence ||
						$scope.friend.joinable !== socialPresence.joinable ||
						$scope.friend.ingame !== socialPresence.ingame ||
						$scope.friend.broadcasting !== socialPresence.broadcasting) {

						$scope.friend.presence = socialPresence.presence;
						$scope.friend.joinable = socialPresence.joinable;
						$scope.friend.ingame = socialPresence.ingame;
						$scope.friend.broadcasting = socialPresence.broadcasting;

						if ((typeof $scope.friend.avatarImgSrc === 'undefined') || ($scope.friend.avatarImgSrc === ComponentsConfigFactory.getImagePath('social//avatar_placeholder.png'))) {
							requestAvatar();
						}
						
						callThrottledScopeDigest();
					}
                }
            });
        }
		//console.log('TR: register for presence change event: ' + $scope.friend.originId);
		//console.log('TR: presence: ' + JSON.stringify($scope.friend.presence));

        handles[handles.length] = RosterDataFactory.events.on('socialPresenceChanged:' + $scope.friend.nucleusId, handlePresenceChange);

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

				handlePresenceChange();

            }
        }

        handles[handles.length] = RosterDataFactory.events.on('updateFriendsDirective' + $scope.friend.nucleusId, handleUpdate);

        function handleNameUpdate(friend) {
			$scope.friend.firstName = (friend.firstName ? friend.firstName : $scope.friend.firstName);
			$scope.friend.lastName = (friend.lastName ? friend.lastName : $scope.friend.lastName);
            $scope.$digest();
        }

        handles[handles.length] = UserDataFactory.events.on('socialFriendsFullNameUpdated:' + $scope.friend.nucleusId, handleNameUpdate);

        /*
        function handleFriendElementCollection() {
            //console.log('TR: handleFriendElementCollection');
            RosterDataFactory.setFriendElementCollection($('.origin-social-hub-roster-friend'));
        }
        RosterDataFactory.events.on('updateFriendElementCollection', handleFriendElementCollection);
        */

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
            ConversationFactory.openConversation($scope.friend.nucleusId);
            Origin.voice.joinVoice($scope.friend.nucleusId, [$scope.friend.nucleusId]);
            $scope.isContextmenuVisible = false;
        };

        $scope.contextmenuCallbacks.viewProfile = function () {
            var requestParams = window.extraParams;

            // Open OIG profile window
            // Check with desing to see if we always do this
            if (requestParams && (requestParams.indexOf('oigwindowid') !== -1)  && Origin.client.isEmbeddedBrowser()) {
                    Origin.client.oig.openIGOProfile($scope.friend.nucleusId);
            }
            else {
                //Check with design to see if we always navigate on the main SPA
            NavigationFactory.goUserProfile($scope.friend.nucleusId);
            $scope.isContextmenuVisible = false;
            }
        };

        $scope.contextmenuCallbacks.joinGame = function () {

            var nucleusId = $scope.friend.nucleusId;
            var productId = $scope.friend.presence.gameActivity.productId;
            var multiplayerId = $scope.friend.presence.gameActivity.multiplayerId;
            var gameTitle = $scope.friend.presence.gameActivity.title;
            JoinGameFactory.joinFriendsGame(nucleusId, productId, multiplayerId, gameTitle);

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
            if(!$scope.friend.joinable) {
                DialogFactory.openPrompt({
                    id: 'social-hub-roster-inviteToGame',
                    title: 'TEMP: Invite To Game',
                    description: 'This is a temporary dialogue. It will not work!',
                    acceptText: 'Invite to Game',
                    acceptEnabled: true,
                    rejectText: 'Cancel',
                    defaultBtn: 'yes'
                });

                $scope.isContextmenuVisible = false;
            }
        };

		$scope.removeFriendAndBlockDlgStrings = {};
        $scope.removeFriendAndBlockDlgStrings.title = UtilFactory.getLocalizedStr($scope.unfriendandblocktitle, CONTEXT_NAMESPACE, 'unfriendandblocktitle');
		$scope.removeFriendAndBlockDlgStrings.description = UtilFactory.getLocalizedStr($scope.unfriendandblockdescription, CONTEXT_NAMESPACE, 'unfriendandblockdescription');
		$scope.removeFriendAndBlockDlgStrings.description2 = UtilFactory.getLocalizedStr($scope.unfriendandblockdescription2, CONTEXT_NAMESPACE, 'unfriendandblockdescription_2');
		$scope.removeFriendAndBlockDlgStrings.description3 = UtilFactory.getLocalizedStr($scope.unfriendandblockdescription3, CONTEXT_NAMESPACE, 'unfriendandblockdescription_3');
		$scope.removeFriendAndBlockDlgStrings.acceptText = UtilFactory.getLocalizedStr($scope.unfriendandblockaccept, CONTEXT_NAMESPACE, 'unfriendandblockaccept');
		$scope.removeFriendAndBlockDlgStrings.rejectText = UtilFactory.getLocalizedStr($scope.unfriendandblockreject, CONTEXT_NAMESPACE, 'unfriendandblockreject');

		this.handleRemoveFriendAndBlock = function(result) {
			AppCommFactory.events.off('dialog:hide', self.handleRemoveFriendAndBlock);
			if (result.accepted) {
				RosterDataFactory.removeFriendAndBlock($scope.friend.nucleusId);
			}
		};

        $scope.contextmenuCallbacks.removeFriendAndBlock = function () {
			$scope.rosterContextmenuVisible = false;

			var description = '<b>' + $scope.removeFriendAndBlockDlgStrings.description + '</b>' + '<br><br>' +
				$scope.removeFriendAndBlockDlgStrings.description2 + '<br><br>' +
				$scope.removeFriendAndBlockDlgStrings.description3;
			description = description.replace(/\'/g, "&#39;").replace(/%username%/g, $scope.friend.originId);

            DialogFactory.openPrompt({
                id: 'social-hub-roster-removeFriendAndBlock-id',
				name: 'social-hub-roster-removeFriendAndBlock',
				title: $scope.removeFriendAndBlockDlgStrings.title,
                description: description,
				acceptText: $scope.removeFriendAndBlockDlgStrings.acceptText,
				acceptEnabled: true,
                rejectText: $scope.removeFriendAndBlockDlgStrings.rejectText,
				defaultBtn: 'yes'
            });

			AppCommFactory.events.on('dialog:hide', self.handleRemoveFriendAndBlock);

            $scope.isContextmenuVisible = false;

        };

		this.handleRemoveFriend = function(result) {
			AppCommFactory.events.off('dialog:hide', self.handleRemoveFriend);
			if (result.accepted) {
				RosterDataFactory.removeFriend($scope.friend.nucleusId);
			}
		};

		this.onContextMenu = function () {
		    // Check our presence to see if we are in a joinable game
		    var presence = RosterDataFactory.getCurrentUserPresence();
		    if (presence && !!presence.joinable) {
		        $scope.playingJoinableGame = true;
		    }
		};

		$scope.removeFriendDlgStrings = {};
        $scope.removeFriendDlgStrings.title = UtilFactory.getLocalizedStr($scope.unfriendusertitle, CONTEXT_NAMESPACE, 'unfriendusertitle');
		$scope.removeFriendDlgStrings.description = UtilFactory.getLocalizedStr($scope.unfrienduserdescription, CONTEXT_NAMESPACE, 'unfrienduserdescription');
		$scope.removeFriendDlgStrings.acceptText = UtilFactory.getLocalizedStr($scope.unfrienduseraccept, CONTEXT_NAMESPACE, 'unfrienduseraccept');
		$scope.removeFriendDlgStrings.rejectText = UtilFactory.getLocalizedStr($scope.unfrienduserreject, CONTEXT_NAMESPACE, 'unfrienduserreject');

        $scope.contextmenuCallbacks.removeFriend = function () {
            DialogFactory.openPrompt({
                id: 'social-hub-roster-removeFriend-id',
				name: 'social-hub-roster-removeFriend',
				title: $scope.removeFriendDlgStrings.title,
                description: $scope.removeFriendDlgStrings.description.replace('%username%', $scope.friend.originId),
				acceptText: $scope.removeFriendDlgStrings.acceptText,
				acceptEnabled: true,
                rejectText: $scope.removeFriendDlgStrings.rejectText,
				defaultBtn: 'yes'
            });

			AppCommFactory.events.on('dialog:hide', self.handleRemoveFriend);
        };

		function onClientOnlineStateChanged(onlineObj) {
			if (!!onlineObj && !onlineObj.isOnline) {
				DialogFactory.close('social-hub-roster-removeFriend-id');
				DialogFactory.close('social-hub-roster-removeFriendAndBlock-id');
			}
		}

        AuthFactory.events.on('myauth:clientonlinestatechangeinitiated', onClientOnlineStateChanged);

        function onDestroy() {
            RosterDataFactory.events.off('socialPresenceChanged:' + $scope.friend.nucleusId, handlePresenceChange);
            RosterDataFactory.events.off('updateFriendsDirective' + $scope.friend.nucleusId, handleUpdate);
            UserDataFactory.events.off('socialFriendsFullNameUpdated:' + $scope.friend.nucleusId, handleNameUpdate);
            AuthFactory.events.off('myauth:clientonlinestatechangeinitiated', onClientOnlineStateChanged);
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
     * @param {LocalizedString} unfriendandblockaccept Accept prompt of the Unfriend And Block dialog
     * @param {LocalizedString} unfriendandblockreject Reject prompt of the Unfriend And Block dialog
     * @param {LocalizedString} onlinestr "Online" label
     * @param {LocalizedString} awaystr "Away" label
     * @param {LocalizedString} offlinestr "Offline" label
     * @param {LocalizedString} broadcasting "Watch Broadcast" tooltip
     * @param {LocalizedString} joingame "Join Game" tooltip
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
	 *            broadcasting="Watch Broadcast"
	 *            joingame="Join Game"
     *         ></origin-social-hub-roster-friend>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialHubRosterFriendCtrl', OriginSocialHubRosterFriendCtrl)
        .directive('originSocialHubRosterFriend', function (ComponentsConfigFactory, AppCommFactory) {

            return {
                restrict: 'E',
                controller: 'OriginSocialHubRosterFriendCtrl',
                require: ['^originSocialHubRosterSection', 'originSocialHubRosterFriend', '^originSocialHubKeyeventtargetrosteritem'],
                scope: {
                    'friend': '=',
                    'index': '=',
					'unfriendusertitle': '=',
					'unfrienduserdescription': '=',
					'unfrienduseraccept': '=',
					'unfrienduserreject': '=',
					'unfriendandblocktitle': '=',
					'unfriendandblockdescription': '=',
					'unfriendandblockaccept': '=',
					'unfriendandblockreject': '=',
					'onlineStr': '@onlinestr',
					'offlineStr': '@offlinestr',
					'awayStr': '@awaystr',
					'broadcasting': '@broadcasting',
					'joingame': '@joingame'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/hub/roster/views/friend.html'),
                link: function(scope, element, attrs, ctrl) {
                    attrs = attrs;

                    scope.sectionController = ctrl[0];
                    var thisController = ctrl[1],
                        keyeventRosteritemCtrl = ctrl[2];

                    if (panelHeight === 0) {
                        thisController.setElement($(element).find('.origin-social-hub-roster-friend'));
                        thisController.setPanelHeight();
                    }

                    $(element).on('dblclick', '.origin-social-hub-roster-friend', function() {
                        thisController.openConversation();
                    });

                    $(element).on('click', '.origin-social-hub-roster-friend-watchbroadcast-icon', function() {
                        scope.contextmenuCallbacks.watchBroadcast();
                    });

                    $(element).on('click', '.origin-social-hub-roster-friend-joingame-icon', function () {
                        scope.contextmenuCallbacks.joinGame();
                    });

                    // Set up placeholder avatar
                    if (typeof scope.friend.avatarImgSrc === 'undefined') {
                        scope.friend.avatarImgSrc = ComponentsConfigFactory.getImagePath('social//avatar_placeholder.png');
                    }

                    thisController.initialUpdate();

                    /*
                    * Context menu functions
                    */
                    AppCommFactory.events.on('social:updateContextmenuState', function () {
                        scope.isContextmenuVisible = keyeventRosteritemCtrl.getContextmenuState();
                        if (scope.isContextmenuVisible) {
                            thisController.onContextMenu();
                        }
                    });

                    AppCommFactory.events.on('social:triggerEnterEvent', function (data) {
                        var targetType = data.targetType;

                        if(targetType === 'item') {
                            if($(data.thisItem).find(element).length > 0) {
                                thisController.openConversation();
                            }
                        } else if (targetType === 'contextmenuitem') {
                            if($(element).find(data.thisItem).length > 0) {
                                $(data.thisItem).triggerHandler('click');
                            }
                        }
                    });

                    AppCommFactory.events.on('social:deleteItem', function (data) {
                        if($(data.thisItem).find(element).length > 0) {
                            scope.contextmenuCallbacks.removeFriend();
                        }
                    });
                }
            };

        });

}());
