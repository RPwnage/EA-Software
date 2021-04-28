/**
 * @file home/discovery/tile/config/ogrecentlyplayed.js
 */
(function() {
    'use strict';

    function originHomeDiscoveryTileConfigOgrecentlyplayed(DiscoveryStoryFactory) {

        function originHomeDiscoveryTileConfigOgrecentlyplayedLink(scope, element, attrs, parentCtrl) {
            var attributes = {
                priority: scope.priority,
                diminish: scope.diminish,
                limit: scope.limit,
                dayssinceplayed: scope.dayssinceplayed
            };
            DiscoveryStoryFactory.updateAutoStoryTypeConfig(parentCtrl.bucketId(), 'HT_21', attributes);
        }

        return {
            restrict: 'E',
            require: '^originHomeDiscoveryBucket',
            scope: {
                priority: '@',
                diminish: '@',
                limit: '@',
                dayssinceplayed: '@'
            },
            link: originHomeDiscoveryTileConfigOgrecentlyplayedLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileConfigOgrecentlyplayed
     * @restrict E
     * @element ANY
     * @scope
     * @param {number} priority The priority assigned via CQ5 to this type
     * @param {number} diminish The value to diminish the priority by after this type is used
     * @param {number} limit The maximum number of times this tile type can appear
     * @param {number} dayssinceplayed the number of days since the user last played
     * @description
     *
     * tile config for owned game recently played discovery tile
     *
     */
    angular.module('origin-components')
        .directive('originHomeDiscoveryTileConfigOgrecentlyplayed', originHomeDiscoveryTileConfigOgrecentlyplayed);
}());