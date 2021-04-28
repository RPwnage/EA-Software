/**
 * @file user/card.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-user-card';

    function OriginUserCardCtrl($scope, $timeout, DialogFactory, RosterDataFactory, UtilFactory, GamesDataFactory, ComponentsLogFactory, ConversationFactory, AppCommFactory) {
        $scope.originId = '';
        $scope.message = UtilFactory.getLocalizedStr($scope.messageStr, CONTEXT_NAMESPACE, 'messagecta');
        $scope.watchGame = UtilFactory.getLocalizedStr($scope.watchGameStr, CONTEXT_NAMESPACE, 'watchgamecta');
        $scope.joinGame = UtilFactory.getLocalizedStr($scope.joinGameStr, CONTEXT_NAMESPACE, 'joingamecta');
        $scope.isPlayingGame = false;
        $scope.isBroadcasting = false;

        /**
         * go to the user's profile
         * @return {void}
         * @method
         */
        $scope.loadProfile = function() {
            AppCommFactory.events.fire('uiRouter:go', 'app.profile.user', {
                'id': $scope.nucleusId
            });
        };

        /**
         * Function that should be used to build out request to load a create message dialog
         * @return {void}
         * @method
         */
        $scope.createMessage = function() {
            ConversationFactory.openConversation($scope.nucleusId);
        };

        /**
         * Function that should be used to build out request to load a create message dialog
         * @return {void}
         * @method
         */
        $scope.joinFriend = function() {
            DialogFactory.openAlert({
                id: 'join-a-friend-s-game',
                title: 'Join Game',
                description: 'When the rest of OriginX is fully functional, you can join your friend\'s game',
                rejectText: 'OK'
            });
        };

        /**
         * Handle the transition when the user stops/starts playing a game
         * @return {void}
         * @method
         */
        function updateUserInfo() {
            var friendInfo = RosterDataFactory.getRosterFriend($scope.nucleusId),
                gameActivity = {};

            if (friendInfo) {
                if (friendInfo.presence) {
                    gameActivity = friendInfo.presence.gameActivity;
                }
            } else {
                ComponentsLogFactory.error('[OriginUserCardCtrl] UpdateUserInfo -- friendInfo was null');
            }

            $scope.isPlayingGame = !!gameActivity && !!gameActivity.productId;
            $scope.$digest();
        }

        /**
         * Get the user's info and bind it
         * @return {void}
         * @method getUserInfo
         */
        this.getUserInfo = function() {
            var clientData = $scope.offerId ? GamesDataFactory.getClientGame($scope.offerId) : {'installed': false};
            $scope.isInstalled = clientData.installed;

            RosterDataFactory.getFriendInfo($scope.nucleusId).then(function (friendInfo) {
                if (friendInfo) {
                    var gameActivity = friendInfo.presence.gameActivity;

                    $scope.originId = friendInfo.originId;
                    $scope.isPlayingGame = !!gameActivity && !!gameActivity.productId;
                    $scope.isBroadcasting = friendInfo.broadcasting;

                    if ($scope.isPlayingGame) {
                        $scope.isJoinable = gameActivity.joinable;
                        $scope.currentlyPlaying = UtilFactory.getLocalizedStr($scope.currentlyPlayingStr, CONTEXT_NAMESPACE, 'currentlyplaying', {
                            '%game%': '<strong>' + gameActivity.title + '</strong>'
                        });
                    }

                    if ($scope.isBroadcasting) {
                        $scope.broadcastingUrl = gameActivity && gameActivity.twitchPresence;
                        $scope.currentlyPlaying = UtilFactory.getLocalizedStr($scope.currentlyBroadcastingStr, CONTEXT_NAMESPACE, 'currentlybroadcasting', {
                            '%game%': '<strong>' + gameActivity.title + '</strong>'
                        });
                    }

                    if ($scope.offerId) {
                        RosterDataFactory.events.on('socialPresenceChanged:' + $scope.nucleusId, updateUserInfo);
                        $scope.$on('$destroy', function() {
                            RosterDataFactory.events.off('socialPresenceChanged:' + $scope.nucleusId, updateUserInfo);
                        });
                    }
                }
            }).catch(function(error) {
                ComponentsLogFactory.error('[OriginUserCardCtrl] getUserInfo -- friendInfo was null', error);
            });
        };

    }

    function originUserCard(ComponentsConfigFactory) {

        function originUserCardLink(scope, element, args, ctrl) {
            ctrl.getUserInfo();
        }

        return {
            restrict: 'E',
            scope: {
                'nucleusId': '@nucleusid',
                'offerId': '@offerid',
                'messageStr': '@messagecta',
                'watchGameStr': '@watchgamecta',
                'joinGameStr': '@joingamecta',
                'currentlyPlayingStr': '@currentlyplaying',
                'currentlyBroadcastingStr': '@currentlybroadcasting'
            },
            controller: 'OriginUserCardCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('user/views/card.html'),
            link: originUserCardLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originUserCard
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} nucleusid the nucleusId of the user
     * @param {string} offerid the offerid of the game
     * @param {LocalizedString} messagecta text on button that messages user
     * @param {LocalizedString} watchgamecta text on button that watches game
     * @param {LocalizedString} joingamecta text on button that joins game
     * @param {LocalizedString} currentlyplaying the game a user is playing
     * @param {LocalizedString} currentlybroadcasting the game a user is broadcasting
     * @description
     *
     * user card
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-user-card nucleusid="12305105408" offerid="OFB-EAST:109549060"></origin-user-card>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginUserCardCtrl', OriginUserCardCtrl)
        .directive('originUserCard', originUserCard);
}());