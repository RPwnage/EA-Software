
/** 
 * @file store/promo/scripts/subtitle.js
 */ 
(function(){
    'use strict';

    /* jshint ignore:start */
    var LayoutEnumeration = {
        "hero": "hero",
        "full-width": "full-width",
        "half-width": "half-width"
    };

    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    var LayoutMap = {
        "hero": "l-origin-store-column-hero",
        "full-width": "l-origin-store-column-full",
        "half-width": "l-origin-store-column-half"
    };

    function originStorePromoSubtitle(ComponentsConfigFactory) {
        function originStorePromoSubtitleLink (scope, element) {
            element.addClass(LayoutMap[scope.layout]).addClass('origin-store-promo');
        }

        return {
            restrict: 'E',
            scope: {
                layout: '@',
                image : '@',
                title : '@',
                text : '@',
                href : '@',
                ctaText: '@',
                videoid : '@',
                restrictedage: '@',
                startColor: '@',
                endColor: '@',
                startDate: '@',
                endDate: '@',
                availableText: '@',
                offerid: '@',
                showPackart: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/promo/views/subtitle.html'),
            link: originStorePromoSubtitleLink
        };
    }
    
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePromoSubtitle
     * @restrict E
     * @element ANY
     * @scope
     * @description Mercahndizes a promo module with a title/subtitle as its content
     * @param {LayoutEnumeration} layout The type of layout this promo will have.
     * @param {ImageUrl} image The background image for this module.
     * @param {LocalizedString} title Promo title
     * @param {LocalizedString} text Promo subtitle
     * @param {Url} href Url that this module links to
     * @param {LocalizedString} cta-text Text for the call to action. Ex. "Learn More" (Optional).
     * @param {Video} videoid The youtube video ID of the video for this promo (Optional).
     * @param {string} restrictedage The Restricted Age of the youtube video for this promo (Optional).
     * @param {string} start-color The start of the fade color for the background
     * @param {string} end-color The end color/background color of the module
     * @param {DateTime} start-date  Date the offer may be release or go on sale (Optional).
     * @param {DateTime} end-date Date the offer may be available till date or sale date (Optional).
     * @param {LocalizedString} available-text The string that holds the dates. The dates will subbed in the place of %startDate% and %endDate%. EG: "Available %startDate% - %endDate% would yeild "Available 6/10/2015 - 6/15/2015" (Optional).
     * @param {OfferId} offerid the offer id
     * @param {BooleanEnumeration} show-packart Determines whether the pack art from the offerid will be displayed
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-promo-subtitle
     *         layout="hero"
     *         image="http://docs.x.origin.com/origin-x-design-prototype/images/store/wide-ggg.jpg"
     *         title="We Stand By Our Games."
     *         text="If you don't love it, return it."
     *         href="app.store.wrapper.bundle-blackfriday"
     *         cta-text="Learn Stuff"
     *         videoid="wS7gfJqFoA"
     *         restrictedage="18"
     *         start-color="rgba(30, 38, 44, 0)"
     *         end-color="rgba(30, 38, 44, 1)"
     *         start-date="June 10 2015 00:00:00 PST"
     *         end-date="June 15 2015 01:00:00 EST"
     *         available-text="Available %startDate% - %endDate%">
     *     </origin-store-promo-subtitle>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePromoSubtitle', originStorePromoSubtitle);
}()); 
