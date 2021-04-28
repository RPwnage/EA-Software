/**
 * @file store/carousel/hero.js
 */
(function () {
    'use strict';
    /* jshint ignore:start */
    // Enumerations
    var autoRotateEnumeration = {
        "true": true,
        "false": false
    };

    var alignmentEnumeration = {
        "right": "carousel-caption-right",
        "center": "carousel-caption-center",
        "left": "carousel-caption-left"
    };

    /**
     * Show or Hide The social media icons
     * @readonly
     * @enum {string}
     */
    var ShowSocialMediaEnumeration = {
        "true": "true",
        "false": "false"
    };

    /* jshint ignore:end */
    function originStoreCarouselHero(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            transclude: true,
            scope: false,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/carousel/views/hero.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreCarouselHero
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * The store's carousel hero
     * @example
     * @param {alignmentEnumeration} alignment - How the carousel text should be aligned affects every slide
     * @param {number} stationary-period - The number of milliseconds to sit on a slide without changing to the next slide
     * @param {autoRotateEnumeration} auto-rotate - boolean as to if auto rotation should be on or off.
     * @param {ShowSocialMediaEnumeration} showsocialmedia - If social media icons should be shown.
     *
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-store-carousel-hero>
     *               <some-type-of-banner></some-type-of-banner>
     *               <some-type-of-banner></some-type-of-banner>
     *               <some-type-of-banner></some-type-of-banner>
     *           </origin-store-carousel-hero>
     *      </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreCarouselHero', originStoreCarouselHero);
}());
