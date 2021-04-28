/**
 * @file /ownedgamecount.js
 */

(function() {
    'use strict';

    function OriginHomeStatelistenerCtrl($scope, $controller, FeedFactory) {

        $scope.loginEventConfig = {
            obj: FeedFactory,
            eventName: 'authStateChanged'
        };

        $controller('OriginAuthstatelistenerCtrl', {
            $scope: $scope
        });
    }

    function originHomeStatelistener() {
        return {
            restrict: 'A',
            controller: 'OriginHomeStatelistenerCtrl'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeStatelistener
     * @restrict A
     * @element ANY
     * @scope
     * @description
     *
     * listens for online and login state changes for home and sets scope accordingly to turn on/off directives
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <ANY ng-if="isLoading && isOnline" origin-home-statelistener></ANY>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginHomeStatelistenerCtrl', OriginHomeStatelistenerCtrl)
        .directive('originHomeStatelistener', originHomeStatelistener);
}());