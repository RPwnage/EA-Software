/**
 * @file home/discovery/tile/mostpopulargame.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-home-discovery-tile-mostpopulargame';

    function originHomeDiscoveryTileMostpopulargame(ComponentsConfigFactory, UtilFactory) {
        function originHomeDiscoveryTileMostpopulargameLink(scope, element, attrs, ctrl) {
            //tile specific string fallback
            var description = UtilFactory.getLocalizedStr(scope.descriptionStr, CONTEXT_NAMESPACE, 'description');
            scope.description = description ? description + ' ' : description;
            scope.cta = UtilFactory.getLocalizedStr(scope.cta, CONTEXT_NAMESPACE, 'ctatext');
            ctrl.init();
        }

        return {
            restrict: 'E',
            scope: {
                descriptionstr: '@description',
                image: '@',
                offerId: '@offerid',
                cta: '@ctatext'
            },
            controller: 'OriginHomeDiscoveryTileGameCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/discovery/tile/views/game.html'),
            link: originHomeDiscoveryTileMostpopulargameLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileMostpopulargame
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString|OCD} description the tile description
     * @param {LocalizedString} ctatext the call to action text
     * @param {ImageUrl} image  image for the tile
     * @param {string} offerid offer id for the tile
     * @description
     *
     * most popular game discovery tile
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-discovery-tile-mostpopulargame offerid="OFB-EAST:109552316" imageurl="https://eaassets-a.akamaihd.net/origin-com-store-final-assets-prod/76889/231.0x326.0/1007077_LB_231x326_en_US_%5E_2014-08-26-14-16-37_dc204da02063323b31b517ee74425c49b2e2a4ae.png"> </origin-home-discovery-tile-mostpopulargame>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originHomeDiscoveryTileMostpopulargame', originHomeDiscoveryTileMostpopulargame);
}());