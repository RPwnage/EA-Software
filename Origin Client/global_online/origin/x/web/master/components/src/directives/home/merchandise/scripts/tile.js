/**
 * @file home/merchandise/tile.js
 */
(function() {
    'use strict';

    /**
     * Home Merchandise Tile directive
     * @directive originHomeMerchandiseTile
     */

    /* jshint ignore:start */
    var ShowPricingEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    function originHomeMerchandiseTile(ComponentsUrlsFactory, ComponentsConfigFactory, UtilFactory) {
        return {
            restrict: 'E',
            transclude: true,
            scope: {
                imagePath: '@imagepath',
                headline: '@',
                text: '@',
                ctatext: '@',
                offerId: '@offerid'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/merchandise/views/tile.html'),
            link: function(scope, element, attrs) {
                scope.showpricing = attrs.showpricing && (attrs.showpricing.toLowerCase() === 'true');
                scope.image = ComponentsUrlsFactory.getCMSImageUrl(scope.imagePath);
                scope.isTouchDisable = !UtilFactory.isTouchEnabled();
            }
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeMerchandiseTile
     * @restrict E
     * @element ANY
     * @scope
     * @param {ImageUrl} imagepath image in background
     * @param {LocalizedString} headline title text
     * @param {LocalizedString} text description text
     * @param {LocalizedString} ctatext button text
     * @param {ShowPricingEnumeration} showpricing show pricing information
     * @param {string} offerid the offerid related to the promo
     * 
     * @description
     *
     * merchandise tile
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-merchandise-tile headline="Go All In. Save 30% On Premium." text="Upgrade to all maps, modes, vehicles and more for one unbeatable price." ctatext="Save Now" image="https://review.assets.cms.origin.com/content/dam/eadam/Promo-Manager/BF4Premium30off/1007968_opm_690x380_en_US_buynow.jpg" alignment="right" theme="dark" class="ng-isolate-scope"></origin-home-merchandise-tile>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originHomeMerchandiseTile', originHomeMerchandiseTile);

}());