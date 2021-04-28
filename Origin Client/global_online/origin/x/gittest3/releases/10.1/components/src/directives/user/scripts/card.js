/**
 * @file user/card.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-user-card';

    function OriginUserCardCtrl($scope, $timeout, DialogFactory, RosterDataFactory, UtilFactory, ComponentsLogFactory) {
        $scope.originId = '';
        $scope.message = UtilFactory.getLocalizedStr($scope.messageStr, CONTEXT_NAMESPACE, 'messagecta');
        $scope.joinGame = UtilFactory.getLocalizedStr($scope.joinGameStr, CONTEXT_NAMESPACE, 'joingamecta');
        $scope.isPlayingGame = false;
        $scope.isJoinable = false;

        /**
         * Function dismisses the dialog and loads the profile page of a user
         * @return {void}
         * @method
         */
        $scope.loadProfile = function() {
            DialogFactory.close();
            window.location = '#/profile';
        };

        /**
         * Function that should be used to build out request to load a create message dialog
         * @return {void}
         * @method
         */
        $scope.createMessage = function() {
            DialogFactory.openAlert({
                id: 'web-send-friend-a-message',
                title: 'Send Message',
                description: 'When the rest of OriginX is fully functional, you can send a friend a message.',
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

            if(friendInfo) {
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
            var friendInfo = RosterDataFactory.getRosterFriend($scope.nucleusId),
                gameActivity = friendInfo.presence.gameActivity;
            $scope.originId = friendInfo.originId;
            $scope.isPlayingGame = !!gameActivity && !!gameActivity.productId;
            if ($scope.isPlayingGame) {
                $scope.isJoinable = gameActivity.joinable;
                $scope.currentlyPlaying = UtilFactory.getLocalizedStr($scope.currentlyPlayingStr, CONTEXT_NAMESPACE, 'currentlyplaying', {
                    '%game%': '<strong>' + gameActivity.title + '</strong>'
                });
            }
            if ($scope.offerId) {
                RosterDataFactory.events.on('socialFriendsPlayingListUpdated:' + $scope.offerId, updateUserInfo);
                $scope.$on('$destroy', function() {
                    RosterDataFactory.events.off('socialFriendsPlayingListUpdated:' + $scope.offerId, updateUserInfo);
                });
            }
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
                'joinGameStr': '@joingamecta',
                'currentlyPlayingStr': '@currentlyplaying'
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
     * @param {LocalizedString} joingamecta text on button that joins game
     * @param {LocalizedString} currentlyplaying the game a user is playing
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