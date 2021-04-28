/**
 * @file home/discovery/tile/ogrecentlyreleased.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-home-discovery-tile-ogrecentlyreleased';


    function OriginHomeDiscoveryTileOgrecentlyreleasedCtrl($scope, $controller, GamesDataFactory, UtilFactory) {
        /**
         * sets the description for a recently released tile, the description is different if the game is in a preload state
         * @param  {object} catalogInfo [the games catalog info
         */
        function customDescriptionFunction(catalogInfo) {
            var releaseDate = Origin.utils.getProperty(catalogInfo, ['platforms', Origin.utils.os(), 'releaseDate']),
                tokenObj = {
                    '%game%': $scope.gamename,
                    '%releasedate%': releaseDate.toDateString()
                };
            //there should always be a release date, but if some reason there isn't show it as a preload
            if (releaseDate && (Date.now() >= releaseDate)) {
                $scope.description = UtilFactory.getLocalizedStr($scope.descriptionReleased, CONTEXT_NAMESPACE, 'descriptionreleased', tokenObj);

            } else {
                $scope.description = UtilFactory.getLocalizedStr($scope.descriptionPreload, CONTEXT_NAMESPACE, 'descriptionpreload', tokenObj);

            }

        }

        //instantiate the shared controller, but pass in the specific CONTEXT_NAMESPACE and description function (if needed)
        $controller('OriginHomeDiscoveryTileGameCtrl', {
            $scope: $scope,
            contextNamespace: CONTEXT_NAMESPACE,
            customDescriptionFunction: customDescriptionFunction
        });
    }

    function originHomeDiscoveryTileOgrecentlyreleased(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                descriptionPreload: '@descriptionpreload',
                descriptionReleased: '@descriptionreleased',
                imageRaw: '@image',
                gamename: '@',
                offerId: '@offerid'
            },
            controller: 'OriginHomeDiscoveryTileOgrecentlyreleasedCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/discovery/tile/views/game.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileOgrecentlyreleased
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} descriptionpreload preload string for the tile
     * @param {LocalizedString} descriptionreleased released string for the tile
     * @param {LocalizedString|OCD} gamename the game name
     * @param {ImageUrl|OCD} image tile image 1000x250
     * @param {string} offerid offer id for the tile
     *
     * @description
     *
     * recently released discovery tile
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-discovery-tile-ogrecentlyreleased image="https://qa.www.assets.cms.origin.com/content/dam/originx/web/app/home/actions/tile_masseffect3_long.png" offerid="OFB-EAST:57198"></origin-home-discovery-tile-ogrecentlyreleased>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginHomeDiscoveryTileOgrecentlyreleasedCtrl', OriginHomeDiscoveryTileOgrecentlyreleasedCtrl)
        .directive('originHomeDiscoveryTileOgrecentlyreleased', originHomeDiscoveryTileOgrecentlyreleased);
}());