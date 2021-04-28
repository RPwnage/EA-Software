(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginSocialChatwindowConversationPresenceindicatorCtrl($scope, RosterDataFactory) {

        // This directive is intended to be used only for 1:1 conversations, so we can do the following...
        var nucleusId = $scope.conversation.participants[0];

        $scope.user = {};

        RosterDataFactory.getFriendInfo(nucleusId).then(function(user) {
            $scope.user = user;
            if (!!$scope.user.presence) {
                translatePresence($scope.user.presence);
            }
            $scope.$digest();
        });

        function handlePresenceChange() {
            var nucleusId = this;
            RosterDataFactory.getFriendInfo(nucleusId).then(function(user) {
                var pres = user.presence;
                if (!!pres) {
                    $scope.user.presence = pres;
                    translatePresence(pres);
                    $scope.$digest();
                }
            });
        }

        function translatePresence(presence) {
            $scope.user.joinable = false;
            $scope.user.ingame = false;
            $scope.user.broadcasting = false;
            if (presence.show === 'ONLINE') {
                if (presence.gameActivity && presence.gameActivity.title !== undefined && presence.gameActivity.title !=='') {
                    // If we have a gameActivity object and a title we must be playing a game.
                    $scope.user.ingame = true;
                    if (presence.gameActivity.joinable){
                        $scope.user.joinable = true;
                    }

					// If we have a twitch presence we must be broadcasting
					if (!!presence.gameActivity.twitchPresence && presence.gameActivity.twitchPresence.length) {
						$scope.user.broadcasting = true;
					}
                }
            }
        }

        function registerForPresenceEvent(nucleusId) {
            RosterDataFactory.events.on('socialPresenceChanged:' + nucleusId, handlePresenceChange, nucleusId);
        }

        function onDestroy() {
            RosterDataFactory.events.off('socialPresenceChanged:' + nucleusId, handlePresenceChange, nucleusId);
        }

        registerForPresenceEvent(nucleusId);
        $scope.$on('$destroy', onDestroy);
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialChatwindowConversationPresenceindicator
     * @restrict E
     * @element ANY
     * @scope
     * @param {Object} conversation Javascript object representing the conversation
     * @description
     *
     * origin chatwindow -> conversation -> presence indicator
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-social-chatwindow-conversation-presenceindicator
     *            conversation="conversation"
     *         ></origin-social-chatwindow-conversation-presenceindicator>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialChatwindowConversationPresenceindicatorCtrl', OriginSocialChatwindowConversationPresenceindicatorCtrl)
        .directive('originSocialChatwindowConversationPresenceindicator', function(ComponentsConfigFactory) {

            return {
                restrict: 'E',
                controller: 'OriginSocialChatwindowConversationPresenceindicatorCtrl',
                scope: {
                    conversation: '='
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/chatwindow/conversation/views/presenceindicator.html'),
            };

        });
}());

