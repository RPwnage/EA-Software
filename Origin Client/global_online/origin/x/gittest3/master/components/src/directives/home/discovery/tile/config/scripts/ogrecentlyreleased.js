/**
 * @file home/discovery/tile/config/ogrecentlyreleased.js
 */
(function() {
    'use strict';

    function originHomeDiscoveryTileConfigOgrecentlyreleased(DiscoveryStoryFactory) {

        function originHomeDiscoveryTileConfigOgrecentlyreleasedLink(scope, element, attrs, parentCtrl) {
            var attributes = {
                priority: scope.priority,
                diminish: scope.diminish,
                limit: scope.limit,
                dayssincerelease: scope.dayssincerelease
            };
            DiscoveryStoryFactory.updateAutoStoryTypeConfig(parentCtrl.bucketId(), 'HT_02', attributes);
        }

        return {
            restrict: 'E',
            require: '^originHomeDiscoveryBucket',
            scope: {
                priority: '@',
                diminish: '@',
                limit: '@',
                dayssincerelease: '@'
            },
            link: originHomeDiscoveryTileConfigOgrecentlyreleasedLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileConfigOgrecentlyreleased
     * @restrict E
     * @element ANY
     * @scope
     * @param {number} priority The priority assigned via CQ5 to this type
     * @param {number} diminish The value to diminish the priority by after this type is used
     * @param {number} limit The maximum number of times this tile type can appear
     * @param {number} dayssincerelease the max number a days a game has been released to show in this list
     * @description
     *
     * tile config for recently released / preload discovery tile
     *
     */
    angular.module('origin-components')
        .directive('originHomeDiscoveryTileConfigOgrecentlyreleased', originHomeDiscoveryTileConfigOgrecentlyreleased);
}());