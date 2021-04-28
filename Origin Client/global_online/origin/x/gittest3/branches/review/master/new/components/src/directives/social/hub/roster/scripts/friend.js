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
    function OriginSocialHubRosterFriendCtrl($scope, $window, $timeout, RosterDataFactory, UserDataFactory, ConversationFactory, ComponentsConfigFactory) {
        $scope.contextmenuCallbacks = {};
        $scope.chattingGroups = [];
        $scope.rosterContextmenuVisible = false;
        $scope.chatroomContextmenuVisible = false;

        var $element = null;

        this.setElement = function(element) {
            $element = element;
        };

        function isThisFriendVisible() {
            var visible = $scope.sectionController.isFriendVisible($scope.index, panelHeight);
			//console.log('TR: isThisFriendVisible: index: ' + $scope.index + ' ' + $scope.friend.originId + ' ' + visible);
            return visible;
        }


        function handlePresenceChange() {
            RosterDataFactory.getFriendSocialInfo($scope.friend.nucleusId).then(function(socialPresence) {
                if (!!socialPresence.presence) {
                    $scope.friend.presence = socialPresence.presence;
                    $scope.friend.joinable = socialPresence.joinable;
                    $scope.friend.ingame = socialPresence.ingame;
                    $scope.friend.broadcasting = socialPresence.broadcasting;

                    $scope.$digest();
                }
            });
        }
		//console.log('TR: register for presence change event: ' + $scope.friend.originId);
		//console.log('TR: presence: ' + JSON.stringify($scope.friend.presence));

        RosterDataFactory.events.on('socialPresenceChanged:' + $scope.friend.nucleusId, handlePresenceChange);

        function handleUpdate() {
            //console.log('TR: handleUpdate: ' + $scope.friend.originId);

            function requestAvatar() {
                UserDataFactory.getAvatar($scope.friend.nucleusId, 'AVATAR_SIZE_MEDIUM')
                    .then(function(response) {

                        $scope.friend.avatarImgSrc = response;
                        $scope.$digest();
                    }, function() {

                    }).catch(function(error) {
                        Origin.log.exception(error, 'OriginSocialHubRosterFriendCtrl: UserDataFactory.getAvatar failed');
                    });
            }

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

        //RosterDataFactory.events.on('updateRosterViewport', handleUpdate);

        RosterDataFactory.events.on('updateFriendsDirective' + $scope.friend.nucleusId, handleUpdate);

        function handleNameUpdate(friend) {
			$scope.friend.firstName = (friend.firstName ? friend.firstName : $scope.friend.firstName);
			$scope.friend.lastName = (friend.lastName ? friend.lastName : $scope.friend.lastName);
            $scope.$digest();
        }

        UserDataFactory.events.on('socialFriendsFullNameUpdated:' + $scope.friend.nucleusId, handleNameUpdate);

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

        /* Context menu functions
        */
        $scope.contextmenuCallbacks.sendMessage = function () {
            ConversationFactory.openConversation($scope.friend.nucleusId);
            $scope.rosterContextmenuVisible = false;
        };

        $scope.contextmenuCallbacks.inviteToChatroom = function (chatroom) {
            $window.alert('This is the "invite to chatroom" button. I invite you to ' + chatroom.title + ' chatoomroom. Make haste!');
            $scope.rosterContextmenuVisible = false;
        };

        $scope.contextmenuCallbacks.startVoiceChat = function () {
            $window.alert('Voice Chat is not implemented yet');
            $scope.rosterContextmenuVisible = false;
        };

        $scope.contextmenuCallbacks.viewProfile = function () {
            $window.alert('Mirror mirror on the wall, my profile was eaten by super tall Jamal! (coming soon)');
            $scope.rosterContextmenuVisible = false;
        };

        $scope.contextmenuCallbacks.showChatroomDropdown = function () {
            $scope.chatroomContextmenuVisible = !$scope.chatroomContextmenuVisible;
        };

        $scope.contextmenuCallbacks.joinGame = function () {
            $window.alert('Join Game is not currently implemented');
            $scope.rosterContextmenuVisible = false;
        };

        $scope.contextmenuCallbacks.watchBroadcast = function () {
            $window.alert('Watch broadcast is not yet implemented');
            $scope.rosterContextmenuVisible = false;
        };

        $scope.contextmenuCallbacks.inviteToGame = function () {
            $window.alert('Invite To Game is not currently implemented');
            $scope.rosterContextmenuVisible = false;
        };

        $scope.contextmenuCallbacks.removeFriendAndBlock = function () {
            // RosterDataFactory.removeFriend($scope.friend.nucleusId);
            $window.alert('I removed your friend, but can"t block yet. I recommend a big fence for now.');
            $scope.rosterContextmenuVisible = false;
        };

        $scope.contextmenuCallbacks.removeFriend = function () {
            //  RosterDataFactory.removeFriend($scope.friend.nucleusId);
            $window.alert('I removed your friend. (Just kidding. Can"t do that yet');
            $scope.rosterContextmenuVisible = false;
        };

        // adds to scope list of groups that user is chatting in
        this.getChattingGroups = function () {
            $scope.chattingGroups = [];
            var convos = ConversationFactory.getConversations();

            // avoid loop if friend is not in a chat group
            if(!$.isEmptyObject(convos) ) {
                angular.forEach(convos, function (v, k) {
                    if(convos[k].state === "MULTI_USER") {
                        $scope.chattingGroups.push(v);
                    }
                });
            }
        };

        function onDestroy() {
            RosterDataFactory.events.off('socialPresenceChanged:' + $scope.friend.nucleusId, handlePresenceChange);
            RosterDataFactory.events.off('updateFriendsDirective' + $scope.friend.nucleusId, handleUpdate);
            UserDataFactory.events.off('socialFriendsFullNameUpdated:' + $scope.friend.nucleusId, handleNameUpdate);
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
     *         ></origin-social-hub-roster-friend>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialHubRosterFriendCtrl', OriginSocialHubRosterFriendCtrl)
        .directive('originSocialHubRosterFriend', function(ComponentsConfigFactory) {

            return {
                restrict: 'E',
                controller: 'OriginSocialHubRosterFriendCtrl',
                require: ['^originSocialHubRosterSection', 'originSocialHubRosterFriend'],
                scope: {
                    'friend': '=',
                    'index': '='
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/hub/roster/views/friend.html'),
                link: function(scope, element, attrs, ctrl) {
                    attrs = attrs;

                    scope.sectionController = ctrl[0];

                    if (panelHeight === 0) {
                        ctrl[1].setElement($(element).find('.origin-social-hub-roster-friend'));
                        ctrl[1].setPanelHeight();
                    }

                    /*
                    * Context menu functions
                    */
                    function isSelectedItem () {
                        return $(element).closest(document.activeElement).length > 0;
                    }

                    // Catches event from keyeventtarget .js
                    scope.$on('newSocialSelection', function (e, data) {

                        if (data.triggerSource === 'leftClick' && !scope.rosterContextmenuVisible) {
                            scope.rosterContextmenuVisible = isSelectedItem();
                        } else if (data.triggerSource === 'rightClick') {
                            scope.rosterContextmenuVisible = isSelectedItem();
                            ctrl[1].getChattingGroups();
                        } else {
                            scope.rosterContextmenuVisible = false;
                        }

                        scope.$digest();
                    });

                    // Hides context menu from clicking away
                    $(document).on('click', function (event) {
                        var $target = $(event.target);

                        if(!$target.hasClass('origin-social-hub-roster-contextmenu-otkicon-downarrow')) {
                            scope.rosterContextmenuVisible = false;
                            scope.$digest();
                         }
                    });

                    $(element).on('dblclick', '.origin-social-hub-roster-friend', function() {
                        ctrl[1].openConversation();
                    });

                    // Set up placeholder avatar
                    if (typeof scope.friend.avatarImgSrc === 'undefined') {
                        scope.friend.avatarImgSrc = ComponentsConfigFactory.getImagePath('social//avatar_placeholder.png');
                    }

                    ctrl[1].initialUpdate();

                    scope.$on('openSocialConversationOnEnter', function (e, data) {
                        var focusedFriend = $(data.thisItem[0]).find($(element));
                        if(focusedFriend.length > 0) {
                            ctrl[1].openConversation();
                        }
                    });
                }
            };

        });

}());

