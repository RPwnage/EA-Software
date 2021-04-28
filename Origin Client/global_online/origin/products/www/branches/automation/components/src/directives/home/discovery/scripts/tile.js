/**
 * @file home/discovery/tile.js
 */
(function() {
    'use strict';

    function originHomeDiscoveryTile() {
        return {
            restrict: 'E',
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTile
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * discovery tile container
     *
     */
    angular.module('origin-components')
        .directive('originHomeDiscoveryTile', originHomeDiscoveryTile);
}());