/**
 * @file home/discovery/tile/config/newdlc.js
 */
(function() {
    'use strict';

    function originHomeDiscoveryTileConfigNewdlc(DiscoveryStoryFactory) {

        function originHomeDiscoveryTileConfigNewdlcLink(scope, element, attrs, parentCtrl) {
            var attributes = {
                priority: scope.priority,
                diminish: scope.diminish,
                limit: scope.limit
            };
            DiscoveryStoryFactory.updateAutoStoryTypeConfig(parentCtrl.bucketId(), 'HT_06', attributes);
        }

        return {
            restrict: 'E',
            require: '^originHomeDiscoveryBucket',
            scope: {
                priority: '@',
                diminish: '@',
                limit: '@'
            },
            link: originHomeDiscoveryTileConfigNewdlcLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileConfigNewdlc
     * @restrict E
     * @element ANY
     * @scope
     * @param {number} priority The priority assigned via CQ5 to this type
     * @param {number} diminish The value to diminish the priority by after this type is used
     * @param {number} limit The maximum number of times this tile type can appear
     * @description
     *
     * tile config for new dlc discovery tile
     *
     */
    angular.module('origin-components')
        .directive('originHomeDiscoveryTileConfigNewdlc', originHomeDiscoveryTileConfigNewdlc);
}());