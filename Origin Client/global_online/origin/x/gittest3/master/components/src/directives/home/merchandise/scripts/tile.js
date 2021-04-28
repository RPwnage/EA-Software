/**
 * @file home/merchandise/tile.js
 */
(function() {
    'use strict';
    /* jshint ignore:start */
    /**
     * Theme Id map
     * @enum {string}
     */
    var ThemeEnumeration = {
        "light": "light",
        "dark": "dark"
    };

    /**
     * Alignment options map
     * @enum {string}
     */
    var AlignmentEnumeration = {
        "left": "left",
        "right": "right"
    };
    /* jshint ignore:end */

    /**
    * Home Merchandise Tile directive
    * @directive originHomeMerchandiseTile
    */
    function originHomeMerchandiseTile(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                image: '@',
                headline: '@',
                text: '@',
                ctatext: '@',
                price: '@',
                discount: '@',
                originalPrice: '@originalprice',
                theme: '@',
                alignment: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/merchandise/views/tile.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeMerchandiseTile
     * @restrict E
     * @element ANY
     * @scope
     * @param {ImageUrl} image image in background
     * @param {LocalizedString} headline title text
     * @param {LocalizedString} text description text
     * @param {LocalizedString} ctatext button text
     * @param {string} price current price
     * @param {string} discount discounted price
     * @param {string} originalprice original price
     * @param {ThemeEnumeration} theme light/dark theme hint
     * @param {AlignmentEnumeration} alignment text alignment hint
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
