
/** 
 * @file store/promo/scripts/offerdetails.js
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

    function originStorePromoOfferdetails(ComponentsConfigFactory) {
        function originStorePromoOfferdetailsLink (scope, element) {
            element.addClass(LayoutMap[scope.layout]).addClass('origin-store-promo');
        }

        return {
            restrict: 'E',
            scope: {
                layout: '@',
                image : '@',
                title : '@',
                href : '@',
                ctaText: '@',
                videoid : '@',
                restrictedage: '@',
                startColor: '@',
                endColor: '@',
                offerid: '@',
                showPackart: '@',
                bulletOne: '@',
                bulletTwo: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/promo/views/offerdetails.html'),
            link: originStorePromoOfferdetailsLink
        };
    }
    
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePromoOfferdetails
     * @restrict E
     * @element ANY
     * @description Merchandizes a promo module with offer details as its content.
     * @param {LayoutEnumeration} layout The type of layout this promo will have.
     * @param {ImageUrl} image The background image for this module.
     * @param {LocalizedString} title Promo title
     * @param {Url} href Url that this module links to
     * @param {LocalizedString} cta-text Text for the call to action. Ex. "Learn More" (Optional).
     * @param {Video} videoid The youtube video ID of the video for this promo (Optional).
     * @param {string} restrictedage The Restricted Age of the youtube video for this promo (Optional).
     * @param {string} start-color The start of the fade color for the background
     * @param {string} end-color The end color/background color of the module
     * @param {OfferId} offerid the offer id
     * @param {BooleanEnumeration} show-packart Determines whether the pack art from the offerid will be displayed
     * @param {string} bullet-one First bullet point (Optional)
     * @param {string} bullet-two Second bullet point (Optional)
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-promo-offerdetails
     *         layout="hero"
     *         image="http://docs.x.origin.com/origin-x-design-prototype/images/store/wide-ggg.jpg"
     *         title="We Stand By Our Games."
     *         href="app.store.wrapper.bundle-blackfriday"
     *         cta-text="Learn Stuff"
     *         videoid="wS7gfJqFoA"
     *         restrictedage="18"
     *         start-color="rgba(30, 38, 44, 0)"
     *         end-color="rgba(30, 38, 44, 1)">
     *     </origin-store-promo-offerdetails>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePromoOfferdetails', originStorePromoOfferdetails);
}()); 
