/**
 * @file user/info.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-user-info';

    function OriginUserInfoCtrl($scope, $timeout, RosterDataFactory, UtilFactory, ComponentsLogFactory) {
        $scope.originId = '';
        $scope.message = UtilFactory.getLocalizedStr($scope.messageStr, CONTEXT_NAMESPACE, 'messagecta');
        $scope.joinGame = UtilFactory.getLocalizedStr($scope.joinGameStr, CONTEXT_NAMESPACE, 'joingamecta');
        $scope.isPlayingGame = false;
        $scope.isJoinable = false;

        /**
         * logs any errors from the getFriendInfo call
         * @param  {Error} error the error object
         */
        function handleGetFriendInfoError(error) {
            ComponentsLogFactory.error('[origin-user-info][getUserInfo]', error.message, error.stack);
        }

        /**
         * take the results of the get friend info an populate the bindings
         * @param  {object} friendInfo the results of the getFriendInfo call
         */
        function populateBindings(friendInfo) {
            $scope.originId = friendInfo.originId;
            $scope.isPlayingGame = !!Object.keys(friendInfo.presence.gameActivity).length;
            if ($scope.isPlayingGame) {
                $scope.isJoinable = friendInfo.presence.gameActivity.joinable;
                $scope.currentlyPlaying = UtilFactory.getLocalizedStr($scope.currentlyPlayingStr, CONTEXT_NAMESPACE, 'currentlyplaying', {
                    '%game%': '<strong>' + friendInfo.presence.gameActivity.title + '</strong>'
                });
            }
        }
        /**
         * Get the user's info and bind it
         * @return {void}
         * @method getUserInfo
         */
        this.getUserInfo = function() {
            RosterDataFactory.getFriendInfo($scope.nucleusId)
                .then(populateBindings)
                .catch(handleGetFriendInfoError);
        };

    }

    function originUserInfo(ComponentsConfigFactory) {

        function originUserInfoLink(scope, element, args, ctrl) {
            ctrl.getUserInfo();
        }

        return {
            restrict: 'E',
            scope: {
                'nucleusId': '@nucleusid',
                'messageStr': '@messagecta',
                'joinGameStr': '@joingamecta',
                'currentlyPlayingStr': '@currentlyplaying'
            },
            controller: 'OriginUserInfoCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('user/views/info.html'),
            link: originUserInfoLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originUserInfo
     * @restrict E
     * @element ANY
     * @param {string} nucleusid the nucleusId of the user
     * @param {LocalizedString} messagecta text on button that messages user
     * @param {LocalizedString} joingamecta text on button that joins game
     * @param {LocalizedString} currentlyplaying the game a user is playing
     * @scope
     * @description
     *
     * userinfo
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-user-info nucleusid="12305105408"></origin-user-info>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginUserInfoCtrl', OriginUserInfoCtrl)
        .directive('originUserInfo', originUserInfo);
}());