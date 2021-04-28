(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginSocialChatwindowConversationParticipantCtrl($scope, ConversationFactory, RosterDataFactory, SocialDataFactory, UtilFactory) {

	    var CONTEXT_NAMESPACE = 'origin-social-chatwindow-conversation-participant';

        $scope.onlineLoc = UtilFactory.getLocalizedStr($scope.onlineStr, CONTEXT_NAMESPACE, 'onlinestr');
        $scope.offlineLoc = UtilFactory.getLocalizedStr($scope.offlineStr, CONTEXT_NAMESPACE, 'offlinestr');
        $scope.awayLoc = UtilFactory.getLocalizedStr($scope.awayStr, CONTEXT_NAMESPACE, 'awaystr');
        $scope.notAFriendLoc = UtilFactory.getLocalizedStr($scope.notAFriendStr, CONTEXT_NAMESPACE, 'notafriendstr');

        $scope.user = {};
        $scope.isSelf = ($scope.participant === Origin.user.userPid()) ? true : false;
        $scope.status = '';
        $scope.userIsAFriend = false;

        function checkIsUserAFriend() {

            $scope.userIsAFriend = false;

            RosterDataFactory.getRoster('ALL').then(function(roster) {
                // Test to see if they are both in the roster and subscription state is BOTH
                if (typeof roster[$scope.participant] !== 'undefined') {
                    if (roster[$scope.participant].subState === 'BOTH') {
                        $scope.userIsAFriend = true;
                        getParticipantUsername();
                    }
                }
            });
        }

        function getParticipantUsername() {
            if ($scope.participant === Origin.user.userPid()) {
                // This is me
                $scope.user.originId = Origin.user.originId();
                $scope.user.presence = RosterDataFactory.getCurrentUserPresence();
                translatePresence($scope.user.presence);
                $scope.status = '';
            }
            else if ($scope.userIsAFriend) {
                // This is a friend
                RosterDataFactory.getFriendInfo($scope.participant).then(function(user) {
                    $scope.user = user;
                    translatePresence(user.presence);
                    $scope.$digest();
                });
            }
            else {
                // This is not a friend
                SocialDataFactory.getGroupMemberInfo($scope.participant).then(function(originId) {
                    $scope.user.originId = originId;
                    $scope.user.presence = { show: 'UNAVAILABLE' };
                    $scope.status = $scope.notAFriendLoc;
                    $scope.$digest();
                });
            }
        }

        function handlePresenceChange() {
            var nucleusId = this;
            if ($scope.participant === Origin.user.userPid()) {
                // This is me
                $scope.user.presence = RosterDataFactory.getCurrentUserPresence();
                translatePresence($scope.user.presence);
                $scope.$digest();
            }
            else if ($scope.userIsAFriend) {
                // This user is one of our friends - update their presence
                RosterDataFactory.getFriendInfo(nucleusId).then(function(user) {
                    var pres = user.presence;
                    if (!!pres) {
                        $scope.user.presence = pres;
                        translatePresence(pres);
                        $scope.$digest();
                    }
                });
            }
            // Else not a friend - we don't display presence
        }

        function translatePresence(presence) {
            $scope.user.joinable = false;
            $scope.user.ingame = false;
            $scope.user.broadcasting = false;
            if (presence.show === 'ONLINE') {
                if (presence.gameActivity && presence.gameActivity.title !== undefined && presence.gameActivity.title !=='') {
                    // If we have a gameActivity object and a title we must be playing a game.
                    $scope.user.ingame = true;
                    $scope.status = presence.gameActivity.title;
                    if (presence.gameActivity.joinable){
                        $scope.user.joinable = true;
                    }

					// If we have a twitch presence we must be broadcasting
					if (!!presence.gameActivity.twitchPresence && presence.gameActivity.twitchPresence.length) {
						$scope.user.broadcasting = true;
					}
                }
            }
            if (!$scope.user.ingame) {
                switch (presence.show) {
                    case 'ONLINE':
                        $scope.status = $scope.onlineLoc;
                        break;
                    case 'AWAY':
                        $scope.status = $scope.awayLoc;
                        break;
                    case 'UNAVAILABLE':
                        $scope.status = $scope.offlineLoc;
                        break;
                }
            }
        }

        function onDestroy() {
            RosterDataFactory.events.off('socialPresenceChanged:' + $scope.participant, handlePresenceChange, $scope.participant);
            RosterDataFactory.events.off('socialRosterUpdated', checkIsUserAFriend);
        }

        $scope.$watch('participant', function(newValue, oldValue) {
            newValue = newValue;
            RosterDataFactory.events.off('socialPresenceChanged:' + oldValue);
            RosterDataFactory.events.on('socialPresenceChanged:' + $scope.participant, handlePresenceChange, $scope.participant);
            checkIsUserAFriend();
            getParticipantUsername();
        });

        RosterDataFactory.events.on('socialRosterUpdated', checkIsUserAFriend);
        $scope.$on('$destroy', onDestroy);
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialChatwindowConversationParticipant
     * @restrict E
     * @element ANY
     * @scope
     * @param {Object} participant Javascript object representing the participant
     * @param {LocalizedString} onlinestr "Online"
     * @param {LocalizedString} offlinestr "Offline"
     * @param {LocalizedString} awaystr "Away"
     * @param {LocalizedString} notafriendstr "Not a friend"
     * @description
     *
     * origin chatwindow -> conversation -> participant list -> participant
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-social-chatwindow-conversation-participant
     *            participant="participant"
     *            onlinestr="Online"
     *            offlinestr="Offline"
     *            awaystr="Away"
     *            notafriendstr="Not a friend"
     *         ></origin-social-chatwindow-conversation-participant>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialChatwindowConversationParticipantCtrl', OriginSocialChatwindowConversationParticipantCtrl)
        .directive('originSocialChatwindowConversationParticipant', function(ComponentsConfigFactory) {

            return {
                restrict: 'E',
                controller: 'OriginSocialChatwindowConversationParticipantCtrl',
                scope: {
                    participant: '=',
                    onlineStr: '@onlinestr',
                    offlineStr: '@offlinestr',
                    awayStr: '@awaystr',
                    notAFriendStr: '@notafriendstr',
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/chatwindow/conversation/views/participant.html'),
            };

        });
}());

