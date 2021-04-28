/**
 * @file home/discovery/tile/ogrecentlyreleased.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-home-discovery-tile-ogrecentlyreleased';

    function originHomeDiscoveryTileOgrecentlyreleased(ComponentsConfigFactory, UtilFactory) {
        function originHomeDiscoveryTileOgrecentlyreleasedLink(scope, element, attrs, ctrl) {
            //tile specific string fallback
            var description = UtilFactory.getLocalizedStr(scope.descriptionStr, CONTEXT_NAMESPACE, 'description');
            scope.description = description ? description + ' ' : description;
            ctrl.init();
        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
                descriptionstr: '@description',
                image: '@',
                offerId: '@offerid'
            },
            controller: 'OriginHomeDiscoveryTileGameCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/discovery/tile/views/game.html'),
            link: originHomeDiscoveryTileOgrecentlyreleasedLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileOgrecentlyreleased
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString|OCD} description description string for the tile
     * @param {ImageUrl|OCD} image image for the tile
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
        .directive('originHomeDiscoveryTileOgrecentlyreleased', originHomeDiscoveryTileOgrecentlyreleased);
}());