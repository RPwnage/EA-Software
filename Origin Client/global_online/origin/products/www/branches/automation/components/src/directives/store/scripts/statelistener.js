/**
 * @file /ownedgamecount.js
 */

(function() {
    'use strict';

    function OriginStoreStatelistenerCtrl($scope, $controller) {

        $controller('OriginAuthstatelistenerCtrl', {
            $scope: $scope
        });
    }

    function originStoreStatelistener() {
        return {
            restrict: 'A',
            controller: 'OriginStoreStatelistenerCtrl'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreStatelistener
     * @restrict A
     * @element ANY
     * @scope
     * @description
     *
     * listens for online state changes for home and sets scope accordingly to turn on/off directives
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <ANY ng-if="isOnline" origin-store-statelistener></ANY>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginStoreStatelistenerCtrl', OriginStoreStatelistenerCtrl)
        .directive('originStoreStatelistener', originStoreStatelistener);
}());