(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginSocialChatwindowConversationBannerCtrl($scope, RosterDataFactory, UtilFactory) {

	    var CONTEXT_NAMESPACE = 'origin-social-chatwindow-conversation-banner';

        $scope.playingLoc = UtilFactory.getLocalizedStr($scope.playingStr, CONTEXT_NAMESPACE, 'playingstr');
        $scope.broadcastingLoc = UtilFactory.getLocalizedStr($scope.broadcastingStr, CONTEXT_NAMESPACE, 'broadcastingstr');
        $scope.joinLoc = UtilFactory.getLocalizedStr($scope.joinStr, CONTEXT_NAMESPACE, 'joinstr');
        $scope.orLoc = UtilFactory.getLocalizedStr($scope.orStr, CONTEXT_NAMESPACE, 'orstr');
        $scope.watchLoc = UtilFactory.getLocalizedStr($scope.watchStr, CONTEXT_NAMESPACE, 'watchstr');

        var nucleusId;

        $scope.state = 'NONE';
        $scope.activeOneToOnePartner = {};

        function translatePresence(presence) {
            $scope.activeOneToOnePartner.joinable = false;
            $scope.activeOneToOnePartner.ingame = false;
            $scope.activeOneToOnePartner.broadcasting = false;
            $scope.activeOneToOnePartner.isPlayingNog = false;
            if (presence.show === 'ONLINE') {
                if (presence.gameActivity && presence.gameActivity.title !== undefined && presence.gameActivity.title !=='') {
                    // If we have a gameActivity object and a title we must be playing a game.
                    $scope.activeOneToOnePartner.ingame = true;

                    if ($scope.activeOneToOnePartner.presence.gameActivity.productId.indexOf('NOG_') > -1) {
                        $scope.activeOneToOnePartner.isPlayingNog = true;
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

        function handlePresenceChange() {
            RosterDataFactory.getFriendInfo(nucleusId).then(function(user) {
                var pres = user.presence;
                if (!!pres) {
                    $scope.activeOneToOnePartner.presence = pres;
                    translatePresence(pres);
                    updateState();
                    $scope.$digest();
                }
            });
        }

        function onDestroy() {
            RosterDataFactory.events.off('socialPresenceChanged:' + nucleusId, handlePresenceChange);
        }

        function registerForPresenceEvent(nucleusId) {
            RosterDataFactory.events.on('socialPresenceChanged:' + nucleusId, handlePresenceChange);
            $scope.$on('$destroy', onDestroy);
        }

        function updateState() {
            if ($scope.conversation.state === 'ONE_ON_ONE') {
                if ($scope.state === 'NONE' && $scope.activeOneToOnePartner.ingame) {
                    $scope.state = 'PLAYING_GAME';
                }
                else if ($scope.state === 'PLAYING_GAME' && !$scope.activeOneToOnePartner.ingame) {
                    $scope.state = 'NONE';
                }
            }
        }

        if ($scope.conversation.state === 'ONE_ON_ONE') {
            nucleusId = $scope.conversation.participants[0];
            registerForPresenceEvent(nucleusId);
            RosterDataFactory.getFriendInfo(nucleusId).then(function(user) {
                $scope.activeOneToOnePartner.presence = user.presence;
                if (!!user.presence) {
                    translatePresence(user.presence);
                }
                updateState();
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
     *         ></origin-social-chatwindow-conversation-banner>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialChatwindowConversationBannerCtrl', OriginSocialChatwindowConversationBannerCtrl)
        .directive('originSocialChatwindowConversationBanner', function(ComponentsConfigFactory, NavigationFactory) {

            return {
                restrict: 'E',
                controller: 'OriginSocialChatwindowConversationBannerCtrl',
                scope: {
                    conversation: '=',
                    playingStr: '@playingstr',
                    broadcastingStr: '@broadcastingstr',
                    joinStr: '@joinstr',
                    orStr: '@orstr',
                    watchStr: '@watchstr'
                },
                link: function(scope, element, attrs, ctrl) {

                    scope = scope;
                    attrs = attrs;
                    ctrl = ctrl;

                    // Clicked on a product link
                    $(element).on('click', '.origin-social-chatwindow-conversation-banner-productlink', function(event) {
                        var productId = $(event.target).attr('data-product-id');
                        if (!!productId) {
                            NavigationFactory.goProductDetails(productId);
                        }
                    });

                    // Clicked "Join"
                    $(element).on('click', '.origin-social-chatwindow-conversation-banner-join', function(event) {
                        event = event;
                        window.alert("FEATURE NOT IMPLEMENTED. Should join game here.");
                    });

                    // Clicked "Watch"
                    $(element).on('click', '.origin-social-chatwindow-conversation-banner-watch', function(event) {
                        event = event;
                        window.alert("FEATURE NOT IMPLEMENTED. Should watch broadcast here.");
                    });

                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/chatwindow/conversation/views/banner.html'),
            };

        });
}());

