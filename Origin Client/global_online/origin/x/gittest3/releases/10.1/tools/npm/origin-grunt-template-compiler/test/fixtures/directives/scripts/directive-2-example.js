/**
 * @file recentlyreleasedtile2.js
 */
(function() {
    'use strict';

    function OriginDiscoveryOgNotPlayedRecentlyTile(ComponentsConfigFactory, UtilFactory) {
        function OriginDiscoveryOgNotPlayedRecentlyTileLink(scope, element, attrs, ctrl) {
            //tile specific string fallback
            var description = UtilFactory.getLocalizedStr(scope.descriptionStr, 'Get back in the game.');
            scope.description = description ? description + ' ' : description;
            ctrl.init();
        }

        return {
            restrict: 'E',
            scope: {
                descriptionstr: '@descriptionstr',
                image: '@',
                offerId: '@offerid'
            },
            controller: 'OriginDiscoveryGameTileCtrl',
            templateUrl: '@app_src/directives/views/game.html',
            link: OriginDiscoveryOgNotPlayedRecentlyTileLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives.originDiscoveryOgNotplayedrecentlyTile
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * not played recently discovery tile
     *
     */
    angular.module('origin-components')
        .directive('originDiscoveryOgNotplayedrecentlyTile', OriginDiscoveryOgNotPlayedRecentlyTile);
}());