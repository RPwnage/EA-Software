/**
 * @file gamelibrary/scripts/friendtile.js
 */
(function () {

    'use strict';

    /**
     * Game Library OGD Friend Tile Ctrl
     * @controller OriginGamelibraryOgdFriendtileCtrl
     */
    function OriginGamelibraryOgdFriendtileCtrl($scope, NavigationFactory, ObserverFactory) {

        /**
          * navigate to the current user's profile
          * 
          * @method viewProfile
          */
        $scope.viewProfile = function () {
            NavigationFactory.goUserProfile($scope.friend.nucleusId);
        };

        ObserverFactory.create($scope.friend._presence)
            .attachToScope($scope, 'presence');
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryOgdFriendtile
     * @restrict E
     * @element ANY
     * @scope
     * @param {Object} friend - the roster friend object
     * @description
     *
     * Game Library - OGD - Friend Tile
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-gamelibrary-ogd-friendtile friend="friend"
     *         ></origin-gamelibrary-ogd-friendtile>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGamelibraryOgdFriendtileCtrl', OriginGamelibraryOgdFriendtileCtrl)
        .directive('originGamelibraryOgdFriendtile', function (ComponentsConfigFactory) {

            return {
                restrict: 'E',
                replace: true,
                controller: 'OriginGamelibraryOgdFriendtileCtrl',
                scope: {
                    friend: '='
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/friendtile.html')
            };

        });
}());

