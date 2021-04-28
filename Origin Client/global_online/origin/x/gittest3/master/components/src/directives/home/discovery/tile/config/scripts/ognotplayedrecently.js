/**
* @file home/discovery/tile/config/ognotplayedrecently.js
*/
(function() {
    'use strict';

    function originHomeDiscoveryTileConfigOgnotplayedrecently(DiscoveryStoryFactory) {

        function originHomeDiscoveryTileConfigOgnotplayedrecentlyLink(scope, element, attrs, parentCtrl) {
            var attributes = {
            priority: scope.priority,
            diminish: scope.diminish,
            limit: scope.limit,
            notplayedlow: scope.notplayedlow,
            notplayedhigh: scope.notplayedhigh
            };
            DiscoveryStoryFactory.updateAutoStoryTypeConfig(parentCtrl.bucketId(), 'HT_03', attributes);
        }

        return {
            restrict: 'E',
            require: '^originHomeDiscoveryBucket',
            scope: {
                priority: '@',
                diminish: '@',
                limit: '@',
                notplayedlow: '@',
                notplayedhigh: '@'
            },
            link: originHomeDiscoveryTileConfigOgnotplayedrecentlyLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileConfigOgnotplayedrecently
     * @restrict E
     * @element ANY
     * @scope
     * @param {number} priority The priority assigned via CQ5 to this type
     * @param {number} diminish The value to diminish the priority by after this type is used
     * @param {number} limit The maximum number of times this tile type can appear
     * @param {number} notplayedlow the lower bound of days not played
     * @param {number} notplayedhigh the upper bound of days not played
     * @description
     *
     * tile config for not recently played discovery tile
     *
     */
    angular.module('origin-components')
        .directive('originHomeDiscoveryTileConfigOgnotplayedrecently', originHomeDiscoveryTileConfigOgnotplayedrecently);
}());