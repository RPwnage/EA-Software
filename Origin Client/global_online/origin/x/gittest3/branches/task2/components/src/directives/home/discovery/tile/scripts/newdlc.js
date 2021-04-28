/**
 * @file home/discovery/tile/newdlc.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-home-discovery-tile-newdlc';

    function originHomeDiscoveryTileNewdlc(ComponentsConfigFactory, UtilFactory) {
        function originHomeDiscoveryTileNewdlcLink(scope, element, attrs, ctrl) {
            //tile specific string fallback
            var description = UtilFactory.getLocalizedStr(scope.descriptionStr, CONTEXT_NAMESPACE, 'description');
            scope.description = description ? description + ' ' : description;
            scope.cta = UtilFactory.getLocalizedStr(scope.cta, CONTEXT_NAMESPACE, 'ctatext');
            ctrl.init();
        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
                descriptionstr: '@description',
                image: '@image',
                name: '@name',
                offerId: '@offerid',
                cta: '@ctatext'
            },
            controller: 'OriginHomeDiscoveryTileGameCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/discovery/tile/views/game.html'),
            link: originHomeDiscoveryTileNewdlcLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileNewdlc
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString|OCD} description string for the tile
     * @param {LocalizedString|OCD} ctatext the the call to action text
     * @param {LocalizedString|OCD} name the game name
     * @param {ImageUrl|OCD} image tile image 1000x250
     * @param {string} offerid offer id for the tile
     *
     * @description
     *
     * new dlc discovery tile
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-discovery-tile-newdlc offerid="OFB-EAST:109550761" image="https://Eaassets-a.akamaihd.net/origin-com-store-final-assets-prod/76889/231.0x326.0/1004841_LB_231x326_en_US_^_2014-01-23-13-45-06_be769fbd3057bde6a82b49b2ab20e2ddeb628f90.jpg"></origin-home-discovery-tile-newdlc>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originHomeDiscoveryTileNewdlc', originHomeDiscoveryTileNewdlc);
}());