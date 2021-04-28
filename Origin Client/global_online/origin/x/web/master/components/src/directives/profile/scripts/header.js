(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginProfileHeaderCtrl($scope, $timeout, UserDataFactory, RosterDataFactory, UtilFactory, AppCommFactory, DialogFactory, AuthFactory, ComponentsConfigFactory, NavigationFactory, SocialHubFactory, SubscriptionFactory, ComponentsLogFactory) {

        var CONTEXT_NAMESPACE = 'origin-profile-header';

        $scope.onlineLoc = UtilFactory.getLocalizedStr($scope.onlineStr, CONTEXT_NAMESPACE, 'onlinestr');
        $scope.awayLoc = UtilFactory.getLocalizedStr($scope.awayStr, CONTEXT_NAMESPACE, 'awaystr');
        $scope.offlineLoc = UtilFactory.getLocalizedStr($scope.offlineStr, CONTEXT_NAMESPACE, 'offlinestr');
        $scope.invisibleLoc = UtilFactory.getLocalizedStr($scope.invisibleStr, CONTEXT_NAMESPACE, 'invisiblestr');
        $scope.playingLoc = UtilFactory.getLocalizedStr($scope.playingStr, CONTEXT_NAMESPACE, 'playingstr');
        $scope.broadcastingLoc = UtilFactory.getLocalizedStr($scope.broadcastingStr, CONTEXT_NAMESPACE, 'broadcastingstr');
        $scope.editoneadotcomLoc = UtilFactory.getLocalizedStr($scope.editoneadotcomStr, CONTEXT_NAMESPACE, 'editoneadotcomstr');
        $scope.sendfriendrequestLoc = UtilFactory.getLocalizedStr($scope.sendfriendrequestStr, CONTEXT_NAMESPACE, 'sendfriendrequeststr');
        $scope.friendrequestsentLoc = UtilFactory.getLocalizedStr($scope.friendrequestsentStr, CONTEXT_NAMESPACE, 'friendrequestsentstr');
        $scope.acceptFriendRequestLoc = UtilFactory.getLocalizedStr($scope.acceptFriendRequestStr, CONTEXT_NAMESPACE, 'acceptfriendrequeststr');
        $scope.ignoreFriendRequestLoc = UtilFactory.getLocalizedStr($scope.ignoreFriendRequestStr, CONTEXT_NAMESPACE, 'ignorefriendrequeststr');
        $scope.unfriendLoc = UtilFactory.getLocalizedStr($scope.unfriendStr, CONTEXT_NAMESPACE, 'unfriendstr');
        $scope.blockandunfriendLoc = UtilFactory.getLocalizedStr($scope.blockandunfriendStr, CONTEXT_NAMESPACE, 'blockandunfriendstr');
        $scope.blockuserLoc = UtilFactory.getLocalizedStr($scope.blockuserStr, CONTEXT_NAMESPACE, 'blockuserstr');
        $scope.unblockuserLoc = UtilFactory.getLocalizedStr($scope.unblockuserStr, CONTEXT_NAMESPACE, 'unblockuserstr');
        $scope.reportuserLoc = UtilFactory.getLocalizedStr($scope.reportuserStr, CONTEXT_NAMESPACE, 'reportuserstr');
        $scope.unfriendusertitleLoc = UtilFactory.getLocalizedStr($scope.unfriendusertitleStr, CONTEXT_NAMESPACE, 'unfriendusertitlestr');
        $scope.unfrienduserdescriptionLoc = UtilFactory.getLocalizedStr($scope.unfrienduserdescriptionStr, CONTEXT_NAMESPACE, 'unfrienduserdescriptionstr');
        $scope.unfrienduseracceptLoc = UtilFactory.getLocalizedStr($scope.unfrienduseracceptStr, CONTEXT_NAMESPACE, 'unfrienduseracceptstr');
        $scope.unfrienduserrejectLoc = UtilFactory.getLocalizedStr($scope.unfrienduserrejectStr, CONTEXT_NAMESPACE, 'unfrienduserrejectstr');
        $scope.errorfriendnotaddedtitleLoc = UtilFactory.getLocalizedStr($scope.errorfriendnotaddedtitle, CONTEXT_NAMESPACE, 'errorfriendnotaddedtitle');
        $scope.errorfriendnotaddeddescriptionLoc = UtilFactory.getLocalizedStr($scope.errorfriendnotaddeddescription, CONTEXT_NAMESPACE, 'errorfriendnotaddeddescription');
        $scope.errorfriendnotaddedacceptLoc = UtilFactory.getLocalizedStr($scope.errorfriendnotaddedaccept, CONTEXT_NAMESPACE, 'errorfriendnotaddedaccept');
        $scope.errorfriendnotaddedcancelLoc = UtilFactory.getLocalizedStr($scope.errorfriendnotaddedcancel, CONTEXT_NAMESPACE, 'errorfriendnotaddedcancel');
        $scope.cancelandblockuserLoc = UtilFactory.getLocalizedStr($scope.cancelandblockuser, CONTEXT_NAMESPACE, 'cancelandblockuser');
        $scope.ignoreandblockuserLoc = UtilFactory.getLocalizedStr($scope.ignoreandblockuser, CONTEXT_NAMESPACE, 'ignoreandblockuser');

        $scope.cancelandblocktitleLoc = UtilFactory.getLocalizedStr($scope.cancelandblocktitle, CONTEXT_NAMESPACE, 'cancelandblocktitle');
        $scope.cancelandblockdescriptionLoc = UtilFactory.getLocalizedStr($scope.cancelandblockdescription, CONTEXT_NAMESPACE, 'cancelandblockdescription');
        $scope.cancelandblockdescription2Loc = UtilFactory.getLocalizedStr($scope.cancelandblockdescription2, CONTEXT_NAMESPACE, 'cancelandblockdescription_2');
        $scope.changeminddescriptionLoc = UtilFactory.getLocalizedStr($scope.changeminddescription, CONTEXT_NAMESPACE, 'changeminddescription');
        $scope.cancelandblockacceptLoc = UtilFactory.getLocalizedStr($scope.cancelandblockaccept, CONTEXT_NAMESPACE, 'cancelandblockaccept');
        $scope.cancelandblockdeclineLoc = UtilFactory.getLocalizedStr($scope.cancelandblockdecline, CONTEXT_NAMESPACE, 'cancelandblockdecline');
        $scope.ignoreandblocktitleLoc = UtilFactory.getLocalizedStr($scope.ignoreandblocktitle, CONTEXT_NAMESPACE, 'ignoreandblocktitle');
        $scope.ignoreandblockdescriptionLoc = UtilFactory.getLocalizedStr($scope.ignoreandblockdescription, CONTEXT_NAMESPACE, 'ignoreandblockdescription');
        $scope.ignoreandblockdescription2Loc = UtilFactory.getLocalizedStr($scope.ignoreandblockdescription2, CONTEXT_NAMESPACE, 'ignoreandblockdescription_2');
        $scope.ignoreandblockacceptLoc = UtilFactory.getLocalizedStr($scope.ignoreandblockaccept, CONTEXT_NAMESPACE, 'ignoreandblockaccept');
        $scope.ignoreandblockdeclineLoc = UtilFactory.getLocalizedStr($scope.ignoreandblockdecline, CONTEXT_NAMESPACE, 'ignoreandblockdecline');

        var needInitialPresence = true;

        $scope.subState = '';
        $scope.relationship = '';
        $scope.presenceStr = '';
        $scope.presenceFlags = {};
        $scope.gameTitle = '';
        $scope.subscription = false;
        $scope.invisible = false;

        $scope.avatarImgSrc = ComponentsConfigFactory.getImagePath('profile\\avatar-placeholder.png');

        $scope.optionsContextmenuVisible = false;
        $scope.contextmenuCallbacks = {};

        function setRelationship() {

            // If the nucleusId was not specified view current user's profile
            if ($scope.nucleusId === '') {
                $scope.nucleusId = Origin.user.userPid();
            }

            if (Number($scope.nucleusId) === Number(Origin.user.userPid())) {
                $scope.relationship = 'SELF';
                if (needInitialPresence) {
                    getInitialPresence();
                }
                $scope.subscription = SubscriptionFactory.userHasSubscription();
                $scope.invisible = RosterDataFactory.getIsInvisible();
            }
            else {
                $scope.relationship = '';
                $scope.subState = 'NONE';
                RosterDataFactory.getRoster('ALL').then(function (roster) {
                    // Test to see if they are both in the roster and subscription state is BOTH
                    if (typeof roster[$scope.nucleusId] !== 'undefined') {
                        $scope.subState = roster[$scope.nucleusId].subState;
                        if ($scope.subState === 'BOTH') {
                            $scope.relationship = 'FRIEND';
                            if (needInitialPresence) {
                                getInitialPresence();
                            }
                        }
                        else {
                            $scope.relationship = 'NON_FRIEND';
                            if (roster[$scope.nucleusId].subReqSent) {
                                $scope.subState = 'PENDING_OUT';
                            }
                            else {
                                $scope.subState = 'PENDING_IN';
                            }
                        }
                    }
                    else {
                        RosterDataFactory.isBlocked($scope.nucleusId).then(function(result) {
                            if ($scope.relationship !== 'SELF') {
								$scope.relationship = result ? 'BLOCKED' : 'NON_FRIEND';
							}
							
                        });
                    }
					
					$scope.$digest();
                });
            }
        }

        function getUserInfo() {

            // If the nucleusId was not specified view current user's profile
            if ($scope.nucleusId === '') {
                $scope.nucleusId = Origin.user.userPid();
            }

            // Figure out our relationship to this user
            setRelationship();

            // Get username, first name and last name
            Origin.atom.atomUserInfoByUserIds([$scope.nucleusId])
                .then(function (response) {
                    $scope.user = response[0];
                    $scope.$digest();
                }).catch(function (error) {
                    ComponentsLogFactory.error('OriginProfileHeaderCtrl: Origin.atom.atomUserInfoByUserIds failed', error);
                });

            // Get the avatar image
            UserDataFactory.getAvatar($scope.nucleusId, 'AVATAR_SZ_LARGE')
                .then(function (response) {
                    $scope.avatarImgSrc = response;
                    $scope.backgroundImage = response;
                    $scope.$digest();
                }, function () {

                }).catch(function (error) {
                    ComponentsLogFactory.error('OriginProfileHeaderCtrl: UserDataFactory.getAvatar failed', error);
                });
			
			// Force reload the blocking list, in case this has changed asynchronously
			RosterDataFactory.loadBlockList();
        }

        function getInitialPresence() {
            if ($scope.relationship === 'SELF') {
                $scope.presence = RosterDataFactory.getCurrentUserPresence();
                setPresenceFlags($scope.presence);
                updatePresenceString();
            }
            else if ($scope.relationship === 'FRIEND') {
                RosterDataFactory.getFriendInfo($scope.nucleusId).then(function (user) {
                    $scope.presence = user.presence;
                    needInitialPresence = false;
                    if (!!user.presence) {
                        setPresenceFlags(user.presence);
                    }
                    updatePresenceString();
                    $scope.$digest();
                });
            }
            // else non friend - leave presence empty
        }

        function setPresenceFlags(presence) {
            $scope.presenceFlags.joinable = false;
            $scope.presenceFlags.ingame = false;
            $scope.presenceFlags.broadcasting = false;
            $scope.gameTitle = '';
            if (presence.show === 'ONLINE') {
                if (presence.gameActivity && presence.gameActivity.title !== undefined && presence.gameActivity.title !== '') {
                    // If we have a gameActivity object and a title we must be playing a game.
                    $scope.presenceFlags.ingame = true;
                    $scope.gameTitle = presence.gameActivity.title;
                    if (presence.gameActivity.joinable) {
                        $scope.presenceFlags.joinable = true;
                    }

                    // If we have a twitch presence we must be broadcasting
                    if (!!presence.gameActivity.twitchPresence && presence.gameActivity.twitchPresence.length) {
                        $scope.presenceFlags.broadcasting = true;
                    }
                }
            }
        }

        function updatePresenceString() {
			if ($scope.presenceFlags.broadcasting && $scope.gameTitle.length) {
				$scope.presenceStr = $scope.broadcastingLoc + ' ' + $scope.gameTitle;
			}
			else if ($scope.presenceFlags.ingame && $scope.gameTitle.length) {
				$scope.presenceStr = $scope.playingLoc + ' ' + $scope.gameTitle;
			}
			else if ($scope.presence.presenceType === 'UNAVAILABLE') {
				if ($scope.relationship === 'SELF') {
					$scope.presenceStr = $scope.invisibleLoc;
				} else {
					$scope.presenceStr = $scope.offlineLoc;
				}
			} 
			else {
				switch ($scope.presence.show) {
					case 'ONLINE':
						$scope.presenceStr = $scope.onlineLoc;
						break;
					case 'AWAY':
						$scope.presenceStr = $scope.awayLoc;
						break;
					case 'UNAVAILABLE':
						if ($scope.relationship === 'SELF') {
							$scope.presenceStr = $scope.invisibleLoc;
							break;
						}
						// Note: if this is not "SELF" then drop through to default. Hence no "break" statement here.
						/* falls through */
					default:
						$scope.presenceStr = $scope.offlineLoc;
						break;
				}				
			}			
        }

        if (AuthFactory.isAppLoggedIn()) {
            getUserInfo();
        }

        function handleBlocklistChange() {
            $timeout(function() { setRelationship(); }, 0);
        }

        $scope.removeFriendAndBlockDlgStrings = {};
        $scope.removeFriendAndBlockDlgStrings.title = UtilFactory.getLocalizedStr($scope.unfriendandblocktitle, CONTEXT_NAMESPACE, 'unfriendandblocktitle');
        $scope.removeFriendAndBlockDlgStrings.description = UtilFactory.getLocalizedStr($scope.unfriendandblockdescription, CONTEXT_NAMESPACE, 'unfriendandblockdescription');
        $scope.removeFriendAndBlockDlgStrings.description2 = UtilFactory.getLocalizedStr($scope.unfriendandblockdescription2, CONTEXT_NAMESPACE, 'unfriendandblockdescription_2');
        $scope.removeFriendAndBlockDlgStrings.description3 = UtilFactory.getLocalizedStr($scope.unfriendandblockdescription3, CONTEXT_NAMESPACE, 'unfriendandblockdescription_3');
        $scope.removeFriendAndBlockDlgStrings.acceptText = UtilFactory.getLocalizedStr($scope.unfriendandblockaccept, CONTEXT_NAMESPACE, 'unfriendandblockaccept');
        $scope.removeFriendAndBlockDlgStrings.rejectText = UtilFactory.getLocalizedStr($scope.unfriendandblockreject, CONTEXT_NAMESPACE, 'unfriendandblockreject');

        $scope.blockDlgStrings = {};
        $scope.blockDlgStrings.title = UtilFactory.getLocalizedStr($scope.blocktitle, CONTEXT_NAMESPACE, 'blocktitle');
        $scope.blockDlgStrings.description = UtilFactory.getLocalizedStr($scope.blockdescription, CONTEXT_NAMESPACE, 'blockdescription');
        $scope.blockDlgStrings.description2 = UtilFactory.getLocalizedStr($scope.blockdescription2, CONTEXT_NAMESPACE, 'blockdescription_2');
        $scope.blockDlgStrings.description3 = UtilFactory.getLocalizedStr($scope.blockdescription3, CONTEXT_NAMESPACE, 'blockdescription_3');
        $scope.blockDlgStrings.acceptText = UtilFactory.getLocalizedStr($scope.blockaccept, CONTEXT_NAMESPACE, 'blockaccept');
        $scope.blockDlgStrings.rejectText = UtilFactory.getLocalizedStr($scope.blockreject, CONTEXT_NAMESPACE, 'blockreject');

        this.handleBlockUser = function(result) {
            AppCommFactory.events.off('dialog:hide', self.handleBlockUser);
            if (result.accepted) {
                RosterDataFactory.blockUser($scope.nucleusId);
            }
        };

        this.handleCancelAndBlockUser = function(result) {
            AppCommFactory.events.off('dialog:hide', self.handleCancelAndBlockUser);
            if (result.accepted) {
                RosterDataFactory.cancelAndBlockUser($scope.nucleusId);
            }
        };

        this.handleIgnoreAndBlockUser = function(result) {
            AppCommFactory.events.off('dialog:hide', self.handleIgnoreAndBlockUser);
            if (result.accepted) {
                RosterDataFactory.ignoreAndBlockUser($scope.nucleusId);
            }
        };

        this.handleUnfriendAndBlockUser = function(result) {
            AppCommFactory.events.off('dialog:hide', self.handleUnfriendAndBlockUser);
            if (result.accepted) {
                RosterDataFactory.removeFriendAndBlock($scope.nucleusId);
            }
        };

        function onClientAuthChanged(authChangeObject) {
            authChangeObject = authChangeObject;

            getUserInfo();

            // listen for presence and roster changes
            RosterDataFactory.events.off('socialPresenceChanged:' + $scope.nucleusId, handlePresenceChange);
            RosterDataFactory.events.off('visibilityChanged', handleVisibilityChange);
            Origin.events.off(Origin.events.XMPP_ROSTERCHANGED, onXmppRosterChanged);
            RosterDataFactory.events.on('socialPresenceChanged:' + $scope.nucleusId, handlePresenceChange);
            RosterDataFactory.events.on('visibilityChanged', handleVisibilityChange);
            Origin.events.on(Origin.events.XMPP_ROSTERCHANGED, onXmppRosterChanged);

            $scope.$digest();
        }

        function onXmppRosterChanged(rosterChangeObject) {

            var jid = (rosterChangeObject.jid.split('@'))[0];
            if (Number(jid) === Number($scope.nucleusId)) {

                getUserInfo();

                if (rosterChangeObject.subReqSent && (rosterChangeObject.subState === 'NONE')) {
                    // We just sent a subscription request
                    $scope.subState = 'PENDING_OUT';					
                }
                else if (rosterChangeObject.subState === 'REMOVE') {
                    // Unfriended
                    $scope.presence = { show: '', presenceType: '' };
                    setPresenceFlags($scope.presence);
                    $scope.presenceStr = '';
                }
                else if (rosterChangeObject.subState === 'BOTH') {
                    // Friended
                    // $timeout to wait for Promise in getUserInfo to resolve
                    // They are not yet in the roster and will show as a NON_FRIEND
                    $timeout(function () {
                        $scope.relationship = 'FRIEND';
                        getInitialPresence();
                    }, 0);
                }

                $scope.$digest();

            }
        }

        function handlePresenceChange(newPresence) {

            // Use a timeout to fix a race condition where a user's presence changes which fires a RosterChanged event
            // which calls getUserInfo which sets the relationship to '' and then waits for a promise before
            // it gets set back to FRIEND, in the meantime the test below fails because they are still ''
            $timeout(function () {
				
				if ( ($scope.relationship === 'SELF') && 
					((newPresence.presenceType === 'UNAVAILABLE') || (newPresence.show === 'OFFLINE'))) {
					$scope.invisible = true;
				} else {
					$scope.invisible = false;
				}
				
                if (($scope.relationship === 'FRIEND') || ($scope.relationship === 'SELF')) {
                    $scope.presence = newPresence;
                    setPresenceFlags(newPresence);
                    updatePresenceString();
                    $scope.$digest();
                }
                else {
                    if (newPresence.presenceType === 'SUBSCRIBE') {
                        // We just received a subscription request
                        $scope.subState = 'PENDING_IN';
                        $scope.$digest();
                    }
                }
            }, 0);

        }

        function handleVisibilityChange(invisible) {
            if ($scope.relationship === 'SELF') {
                $scope.invisible = invisible;
				if (invisible) {
					$scope.presence.show = 'UNAVAILABLE';
				} else {
					$scope.presence = RosterDataFactory.getCurrentUserPresence();
				}				
                updatePresenceString();
                $scope.$digest();
            }
        }

        // listen for auth changes
        AuthFactory.events.on('myauth:change', onClientAuthChanged);
        AuthFactory.events.on('myauth:ready', onClientAuthChanged);

        function onDestroy() {
            RosterDataFactory.events.off('socialPresenceChanged:' + $scope.nucleusId, handlePresenceChange);
            RosterDataFactory.events.off('visibilityChanged', handleVisibilityChange);
            Origin.events.off(Origin.events.XMPP_ROSTERCHANGED, onXmppRosterChanged);
            AuthFactory.events.off('myauth:change', onClientAuthChanged);
            AuthFactory.events.off('myauth:ready', onClientAuthChanged);
            RosterDataFactory.events.off('xmppBlockListChanged', handleBlocklistChange);
        }

        // listen for presence and roster changes
        RosterDataFactory.events.on('socialPresenceChanged:' + $scope.nucleusId, handlePresenceChange);
        RosterDataFactory.events.on('visibilityChanged', handleVisibilityChange);
        Origin.events.on(Origin.events.XMPP_ROSTERCHANGED, onXmppRosterChanged);

        RosterDataFactory.events.on('xmppBlockListChanged', handleBlocklistChange);
        $scope.$on('$destroy', onDestroy);


        // Context Menu Functions
        this.toggleOptionsContextmenu = function () {
            $scope.optionsContextmenuVisible = !$scope.optionsContextmenuVisible;

            if ($scope.optionsContextmenuVisible) {
                // position the menu under the button
                var smallScreen = $(window).width() < 600;
                var left, top, right;
                var $header = $scope.$element.find('.origin-profile-header');
                var paddingRight = parseInt($header.css('padding-right'), 10);
                var paddingTop = parseInt($header.css('padding-top'));
                var $menuButton = $scope.$element.find('.origin-profile-header-controls-options-button');
                var $offsetParent = $menuButton.offsetParent();
                var $menu = $scope.$element.find('#origin-profile-header-controls-options-menu');
                var buttonPosition = $menuButton.position();
                var leftPositioned = $menuButton.css('float') === 'left'
                ;

                // when positioned to left of a button on small screens
                // ie: pending friend requests
                if (leftPositioned) {
                    left = buttonPosition.left + paddingRight;
                    top = (buttonPosition.top + $menuButton.height()) + paddingTop + 40;
                    $menu.css({ 'left': left + 'px', 'top': top + 'px', 'right': 'auto' });
                }
                else {
                    if(smallScreen) {
                        right = ($offsetParent.width() - buttonPosition.left) + paddingRight;
                    } else {
                        right = ($offsetParent.width() - buttonPosition.left) + paddingRight + 70;
                    }
                    top = (buttonPosition.top + $menuButton.height()) + paddingTop + 8;
                    $menu.css({ 'right': right + 'px', 'top': top + 'px', 'left': 'auto' });
                }
            }

            $scope.$digest();
        };

        this.closeOptionsContextmenu = function (digest) {
            $scope.optionsContextmenuVisible = false;
            if(digest) { $scope.$digest(); }
        };

        $(window).resize(function () {
            // If the window size changes close the context menu so the user can't resize it off the screen
            $scope.optionsContextmenuVisible = false;
            $scope.$digest();
        });

        var self = this;
        this.handleRemoveFriend = function (result) {
            AppCommFactory.events.off('dialog:hide', self.handleRemoveFriend);
            if (result.accepted) {
                RosterDataFactory.removeFriend($scope.nucleusId);
                RosterDataFactory.updateRosterViewport();
            }
        };

        $scope.contextmenuCallbacks.ignoreFriendRequest = function () {
            self.closeOptionsContextmenu(false);

            RosterDataFactory.friendRequestReject($scope.nucleusId);
            RosterDataFactory.updateRosterViewport();
        };

        $scope.contextmenuCallbacks.unfriendUser = function() {

            self.closeOptionsContextmenu(false);
            DialogFactory.openPrompt({
                id: 'profile-header-removeFriend',
                title: $scope.unfriendusertitleLoc,
                description: $scope.unfrienduserdescriptionLoc.replace('%username%', $scope.user.EAID),
                acceptText: $scope.unfrienduseracceptLoc,
                acceptEnabled: true,
                rejectText: $scope.unfrienduserrejectLoc,
                defaultBtn: 'yes'
            });

            AppCommFactory.events.on('dialog:hide', self.handleRemoveFriend);
        };

        $scope.contextmenuCallbacks.blockUser = function () {

            self.closeOptionsContextmenu(false);

            var description = '<b>' + $scope.blockDlgStrings.description + '</b>' + '<br><br>' +
                $scope.blockDlgStrings.description2 + '<br><br>' +
                $scope.blockDlgStrings.description3;
            description = description.replace(/\'/g, "&#39;").replace(/%username%/g, $scope.user.EAID);

            DialogFactory.openPrompt({
                id: 'social-profile-header-blockUser',
                title: $scope.blockDlgStrings.title,
                description: description,
                acceptText: $scope.blockDlgStrings.acceptText,
                acceptEnabled: true,
                rejectText: $scope.blockDlgStrings.rejectText,
                defaultBtn: 'yes'
            });

            AppCommFactory.events.on('dialog:hide', self.handleBlockUser);

        };

        $scope.contextmenuCallbacks.cancelAndBlockUser = function () {
            self.closeOptionsContextmenu(false);

            var description = '<b>' + $scope.cancelandblockdescriptionLoc + '</b>' + '<br><br>' +
                $scope.cancelandblockdescription2Loc + '<br><br>' +
                $scope.changeminddescriptionLoc;            
            description = description.replace(/\'/g, "&#39;").replace(/%username%/g, $scope.user.EAID);
        
            DialogFactory.openPrompt({                
                id: 'profile-header-cancelRequestAndBlock-id',
                name: 'profile-header-cancelRequestAndBlock',
                title: $scope.cancelandblocktitleLoc,
                description: description,
                acceptText: $scope.cancelandblockacceptLoc,
                acceptEnabled: true,
                rejectText: $scope.cancelandblockdeclineLoc,
                defaultBtn: 'yes'
            });
            AppCommFactory.events.on('dialog:hide', self.handleCancelAndBlockUser);  
        };
        
        $scope.contextmenuCallbacks.ignoreAndBlockUser = function () {
            self.closeOptionsContextmenu(false);

            var description = '<b>' + $scope.ignoreandblockdescriptionLoc + '</b>' + '<br><br>' +
                $scope.ignoreandblockdescription2Loc + '<br><br>' +
                $scope.changeminddescriptionLoc;            
            description = description.replace(/\'/g, "&#39;").replace(/%username%/g, $scope.user.EAID);
                    
            DialogFactory.openPrompt({                
                id: 'profile-header-ignoreRequestAndBlock-id',
                name: 'profile-header-ignoreRequestAndBlock',
                title: $scope.ignoreandblocktitleLoc,
                description: description,
                acceptText: $scope.ignoreandblockacceptLoc,
                acceptEnabled: true,
                rejectText: $scope.ignoreandblockdeclineLoc,
                defaultBtn: 'yes'
            });
            
            AppCommFactory.events.on('dialog:hide', self.handleIgnoreAndBlockUser);                        
            
        };
                
        $scope.contextmenuCallbacks.blockAndUnfriendUser = function () {

            self.closeOptionsContextmenu(false);

            var description = '<b>' + $scope.removeFriendAndBlockDlgStrings.description + '</b>' + '<br><br>' +
                $scope.removeFriendAndBlockDlgStrings.description2 + '<br><br>' +
                $scope.removeFriendAndBlockDlgStrings.description3;
            description = description.replace(/\'/g, "&#39;").replace(/%username%/g, $scope.user.EAID);

            DialogFactory.openPrompt({
                id: 'social-hub-roster-removeFriendAndBlock',
                title: $scope.removeFriendAndBlockDlgStrings.title,
                description: description,
                acceptText: $scope.removeFriendAndBlockDlgStrings.acceptText,
                acceptEnabled: true,
                rejectText: $scope.removeFriendAndBlockDlgStrings.rejectText,
                defaultBtn: 'yes'
            });

            AppCommFactory.events.on('dialog:hide', self.handleUnfriendAndBlockUser);

        };

        $scope.contextmenuCallbacks.unblockUser = function () {
            RosterDataFactory.unblockUser($scope.nucleusId);
        };

        $scope.contextmenuCallbacks.reportUser = function () {

            self.closeOptionsContextmenu(false);

            NavigationFactory.reportUser($scope.nucleusId, $scope.user.EAID);
        };

        this.editSelfProfile = function () {
            window.alert('Edit on ea.com clicked');
        };

        this.sendFriendRequest = function () {
            RosterDataFactory.sendFriendRequest($scope.nucleusId);
        };

        this.handleErrorDialog = function(result) {
            AppCommFactory.events.off('dialog:hide', self.handleErrorDialog);
            if (result.accepted) {
                SocialHubFactory.unminimizeWindow();
                SocialHubFactory.focusWindow();
            }
        };

        this.acceptFriendRequest = function () {
            RosterDataFactory.friendRequestAccept($scope.nucleusId).then(function() {
                RosterDataFactory.deleteFromSectionRoster('INCOMING', $scope.nucleusId);

            }, function() {

                // Accept friend request failed. Display error dialog
                DialogFactory.openPrompt({
                    id: 'social-hub-roster-errorFriendNotAdded-id',
                    name: 'social-hub-roster-errorFriendNotAdded',
                    title: $scope.errorfriendnotaddedtitleLoc.replace(/\'/g, "&#39;"),
                    description: $scope.errorfriendnotaddeddescriptionLoc,
                    acceptText: $scope.errorfriendnotaddedacceptLoc,
                    acceptEnabled: true,
                    rejectText: $scope.errorfriendnotaddedcancelLoc,
                    defaultBtn: 'yes'
                });

                AppCommFactory.events.on('dialog:hide', self.handleErrorDialog);

            });
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originProfileHeader
     * @restrict E
     * @element ANY
     * @scope
     * @param {number} nucleusId nucleusId of the user
     * @param {LocalizedString} onlinestr "ONLINE"
     * @param {LocalizedString} awaystr "AWAY"
     * @param {LocalizedString} offlinestr "OFFLINE"
     * @param {LocalizedString} invisiblestr "INVISIBLE"
     * @param {LocalizedString} playingstr "PLAYING:"
     * @param {LocalizedString} broadcastingstr "BROADCASTING:"
     * @param {LocalizedString} editoneadotcomstr "Edit on EA.com"
     * @param {LocalizedString} sendfriendrequeststr "Send Friend Request"
     * @param {LocalizedString} friendrequestsentstr "Friend Request Sent"
     * @param {LocalizedString} acceptfriendrequeststr "Accept Friend Request"
     * @param {LocalizedString} ignorefriendrequeststr "Ignore Friend Request"
     * @param {LocalizedString} unfriendstr "Unfriend"
     * @param {LocalizedString} blockandunfriendstr "Block & Unfriend"
     * @param {LocalizedString} blockuserstr "Block User"
     * @param {LocalizedString} unblockuserstr "Unblock User"
     * @param {LocalizedString} reportuserstr "Report User"
     * @param {LocalizedString} unfriendusertitlestr "Unfriend User"
     * @param {LocalizedString} unfrienduserdescriptionstr "Are you sure you want to unfriend %username%?"
     * @param {LocalizedString} unfrienduseracceptstr "Unfriend"
     * @param {LocalizedString} unfrienduserrejectstr "Cancel"
     * @param {LocalizedString} blocktitle title of the Block dialog
     * @param {LocalizedString} blockdescription Description of the Block dialog
     * @param {LocalizedString} blockdescription2 Description of the Block dialog
     * @param {LocalizedString} blockdescription3 Description of the Block dialog
     * @param {LocalizedString} blockaccept Accept prompt of the Block dialog
     * @param {LocalizedString} blockreject Reject prompt of the Block dialog
     * @param {LocalizedString} unfriendandblocktitle title of the Unfriend And Block dialog
     * @param {LocalizedString} unfriendandblockdescription Description of the Unfriend And Block dialog
     * @param {LocalizedString} unfriendandblockdescription2 Description of the Unfriend And Block dialog
     * @param {LocalizedString} unfriendandblockdescription3 Description of the Unfriend And Block dialog
     * @param {LocalizedString} unfriendandblockaccept Accept prompt of the Unfriend And Block dialog
     * @param {LocalizedString} unfriendandblockreject Reject prompt of the Unfriend And Block dialog
     * @param {LocalizedString} errorfriendnotaddedtitle Title of Too Many Friends error dialog
     * @param {LocalizedString} errorfriendnotaddedaccept Accept prompt of Too Many Friends error dialog
     * @param {LocalizedString} errorfriendnotaddeddescription Description of Too Many Friends error dialog
     * @param {LocalizedString} errorfriendnotaddedcancel Reject string Too Many Friends error dialog
     * @param {LocalizedString} cancelandblockuser "Cancel Friend Request and Block"
     * @param {LocalizedString} ignoreandblockuser "Ignore Friend Request and Block"
     * @param {LocalizedString} cancelandblocktitle 
     * @param {LocalizedString} cancelandblockdescription 
     * @param {LocalizedString} cancelandblockdescription2 
     * @param {LocalizedString} changeminddescription
     * @param {LocalizedString} cancelandblockaccept
     * @param {LocalizedString} cancelandblockdecline
     * @param {LocalizedString} ignoreandblocktitle
     * @param {LocalizedString} ignoreandblockdescription
     * @param {LocalizedString} ignoreandblockdescription2
     * @param {LocalizedString} ignoreandblockaccept
     * @param {LocalizedString} ignoreandblockdecline     
     * @description
     *
     * Profile Header
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-profile-header nucleusid="123456789"
     *            onlinestr="ONLINE"
     *            awaystr="AWAY"
     *            offlinestr="OFFLINE"
     *            invisiblestr="INVISIBLE"
     *            playingstr="PLAYING:"
     *            broadcastingstr="BROADCASTING:"
     *            editoneadotcomstr="Edit on EA.com"
     *            sendfriendrequeststr="Send Friend Request"
     *            friendrequestsentstr="Friend Request Sent"
     *            acceptfriendrequeststr="Accept Friend Request"
     *            ignorefriendrequeststr="Ignore Friend Request"
     *            unfriendstr="Unfriend"
     *            blockandunfriendstr="Block & Unfriend"
     *            blockuserstr="Block User"
     *            unblockuserstr="Unblock User"
     *            reportuserstr="Report User"
     *            unfriendusertitlestr="Unfriend User"
     *            unfrienduserdescriptionstr="Are you sure you want to unfriend %username%?"
     *            unfrienduseracceptstr="Unfriend"
     *            unfrienduserrejectstr="Cancel"
     *               blocktitle="Block User"
     *               blockdescription="Are you sure?"
     *               blockaccept="Block"
     *               blockreject="Cancel"
     *              unfriendandblockdescription="Are you sure you want to unfriend and block %username%?"
     *              unfriendandblocktitle="Unfriend And Block"
     *              unfriendandblockaccept="Yes"
     *              unfriendandblockreject="No"
     *               errorfriendnotaddedtitle="Text"
     *               errorfriendnotaddedaccept="Text"
     *               errorfriendnotaddeddescription="Text"
     *               errorfriendnotaddedcancel="Text"
     *               cancelandblockuser= "Cancel Friend Request and Block"
     *               ignoreandblockuser= "Ignore Friend Request and Block"     
     *                 cancelandblocktitle="Text" 
     *                 cancelandblockdescription ="Text"
     *                 cancelandblockdescription2 ="Text"
     *                 cancelandblockaccept="Text"
     *                 cancelandblockdecline="Text"
     *                 ignoreandblocktitle="Text"
     *                 ignoreandblockdescription="Text"
     *                 ignoreandblockdescription2="Text"
     *                 changeminddescription="Text"
     *                 ignoreandblockaccept="Text"
     *                 ignoreandblockdecline="Text"               
     *         ></origin-profile-header>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginProfileHeaderCtrl', OriginProfileHeaderCtrl)
        .directive('originProfileHeader', function (ComponentsConfigFactory) {

            return {
                restrict: 'E',
                controller: 'OriginProfileHeaderCtrl',
                scope: {
                    nucleusId: '@nucleusid',
                    onlineStr: '@onlinestr',
                    awayStr: '@awaystr',
                    offlineStr: '@offlinestr',
                    invisibleStr: '@invisiblestr',
                    playingStr: '@playingstr',
                    broadcastingStr: '@broadcastingstr',
                    editoneadotcomStr: '@editoneadotcomstr',
                    sendfriendrequestStr: '@sendfriendrequeststr',
                    friendrequestsentStr: '@friendrequestsentstr',
                    acceptfriendrequestStr: '@acceptfriendrequeststr',
                    ignorefriendrequestStr: '@ignorefriendrequeststr',
                    unfriendStr: '@unfriendstr',
                    blockandunfriendStr: '@blockandunfriendstr',
                    blockuserStr: '@blockuserstr',
                    unblockuserStr: '@unblockuserstr',
                    reportuserStr: '@reportuserstr',
                    unfriendusertitleStr: '@unfriendusertitlestr',
                    unfrienduserdescriptionStr: '@unfrienduserdescriptionstr',
                    unfrienduseracceptStr: '@unfrienduseracceptstr',
                    unfrienduserrejectStr: '@unfrienduserrejectstr',
                    blocktitleStr: '=',
                    blockdescriptionStr: '=',
                    blockdescription2Str: '=',
                    blockdescription3Str: '=',
                    blockAcceptStr: '=',
                    blockRejectStr: '=',
                    unfriendandblocktitle: '=',
                    unfriendandblockdescription: '=',
                    unfriendandblockaccept: '=',
                    unfriendandblockreject: '=',
                    errorfriendnotaddeddescription: '=',
                    errorfriendnotaddedaccept: '=',
                    errorfriendnotaddedtitle: '=',
                    errorfriendnotaddedcancel: '=',
                    cancelandblockuser: '=',
                    ignoreandblockuser: '=',
                    cancelandblocktitle: '=',
                    cancelandblockdescription: '=',
                    cancelandblockdescription2: '=',
                    changeminddescription: '=',
                    cancelandblockaccept: '=',
                    cancelandblockdecline: '=',
                    ignoreandblocktitle: '=',
                    ignoreandblockdescription: '=',
                    ignoreandblockdescription2: '=',
                    ignoreandblockaccept: '=',
                    ignoreandblockdecline: '='
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('profile/views/header.html'),
                link: function (scope, element, attrs, ctrl) {
                    scope = scope;
                    attrs = attrs;

                    scope.$element = $(element);

                    // Listen for clicks on the primary header button
                    $(element).on('click', '.origin-profile-header-controls-primary-button', function () {
                        var which = $(this).attr('data-button');
                        if (which === 'editselfprofile') {
                            // Edit self profile
                            ctrl.editSelfProfile();
                        }
                        else if (which === 'sendfriendrequest') {
                            // Send friend request
                            ctrl.sendFriendRequest();
                        }
                        else if (which === 'acceptfriendrequest') {
                            // Accept friend request
                            ctrl.acceptFriendRequest();
                        }
                    });


                    // Listen for clicks on the menu button
                    $(element).on('click', '.origin-profile-header-controls-options-button', function () {
                        ctrl.toggleOptionsContextmenu();
                    });

                    // Cancel options menu by clicking elsewhere
                    $(document).on('click', function (e) {
                        var $target = $(e.target);
                        if(scope.optionsContextmenuVisible && !$target.hasClass('origin-profile-header-controls-options-button') ){
                            ctrl.closeOptionsContextmenu(true);
                        }
                    });

                }
            };

        });
}());

