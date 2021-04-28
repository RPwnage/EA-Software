/**
 * @file store/promo/scripts/subtitle-directentitlement.js
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

    var RatingEnumeration = {
        "Light": "light",
        "Dark": "dark"
    };
    /* jshint ignore:end */

    var CONTEXT_NAMESPACE = 'origin-store-promo-subtitle-directentitlement';

    function originStorePromoSubtitle(ComponentsConfigFactory, DirectiveScope) {

        function originStorePromoSubtitleLink (scope, element, attributes, controller) {
            scope.isEntitlement = true;
            scope.showOcdPricing = scope.showPricing === 'true';

            DirectiveScope.populateScopeWithOcdAndCatalog(scope, CONTEXT_NAMESPACE, scope.ocdPath)
                .then(controller.onScopeResolved);
        }

        return {
            restrict: 'E',
            transclude: true,
            scope: {
                layout: '@',
                image : '@',
                titleStr : '@',
                text : '@',
                videoid : '@',
                matureContent: '@',
                startDate: '@',
                endDate: '@',
                availableText: '@',
                ocdPath: '@',
                showPricing: '@',
                showPackart: '@',
                backgroundColor: '@',
                badgeImage: '@',
                ratingFontColor: '@',
                useH1Tag: '@',
                calloutText: '@',
                useDynamicPriceCallout: '@',
                useDynamicDiscountPriceCallout: '@',
                useDynamicDiscountRateCallout: '@',
                othText: '@',
                titleImage: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/promo/views/subtitle.html'),
            controller: 'OriginStorePromoSubtitleCtrl',
            link: originStorePromoSubtitleLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePromoSubtitleDirectentitlement
     * @restrict E
     * @element ANY
     * @scope
     * @description Mercahndizes a promo module with a title/subtitle as its content
     * @param {LayoutEnumeration} layout The type of layout this promo will have.
     * @param {ImageUrl} image The background image for this module.
     * @param {ImageUrl} badge-image optional badge image. Only applicable to hero layout
     * @param {LocalizedString} title-str Promo title
     * @param {LocalizedString} text Promo subtitle
     * @param {Video} videoid The youtube video ID of the video for this promo (Optional).
     * @param {BooleanEnumeration} mature-content Whether the video(s) defined within this component should be considered Mature
     * @param {DateTime} start-date  Date the offer may be release or go on sale (Optional). (UTC time)
     * @param {DateTime} end-date Date the offer may be available till date or sale date (Optional). (UTC time)
     * @param {string} background-color Hex value of the background color
     * @param {LocalizedString} available-text The string that holds the dates. The dates will subbed in the place of %startDate% and %endDate%. EG: "Available %startDate% - %endDate% would yeild "Available 6/10/2015 - 6/15/2015" (Optional).
     * @param {OcdPath} ocd-path OCD Path
     * @param {BooleanEnumeration} show-pricing Show pricing from offer data path. Defaults to false.
     * @param {BooleanEnumeration} show-packart Determines whether the pack art from ocdpath will be displayed
     * @param {RatingEnumeration} rating-font-color color of the ratings font (TW only)
     * @param {String} ctid Content tracking ID
     * @param {BooleanEnumeration} use-h1-tag use H1 Tag for the title to boost SEO relevance. Only one H1 should be present on a page. Optional and defaults to H2
     * @param {LocalizedString} callout-text callout text that will be replaced into placeholder %callout-text% in title str and text
     * @param {BooleanEnumeration} use-dynamic-price-callout flag to use catalog price information that will replace placeholder %~offer-id~%
     * @param {BooleanEnumeration} use-dynamic-discount-price-callout flag to use catalog discount price information that will replace placeholder %~offer-id~%
     * @param {BooleanEnumeration} use-dynamic-discount-rate-callout flag to use catalog discount rate information that will reaplce placeholder %~offer-id~%
     * @param {LocalizedString} oth-text * on the house text.
     * @param {ImageUrl} title-image title image. Will be used only if title str is not merchandised
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-promo-subtitle-directentitlement
     *         layout="hero"
     *         image="https://docs.x.origin.com/proto/images/store/wide-ggg.jpg"
     *         title-str="We Stand By Our Games."
     *         text="If you don't love it, return it."
     *         href="app.store.wrapper.bundle-blackfriday"
     *         videoid="wS7gfJqFoA"
     *         mature-content="true"
     *         start-date="June 10 2015 00:00:00 PST"
     *         end-date="June 15 2015 01:00:00 EST"
     *         available-text="Available %startDate% - %endDate%">
     *     </origin-store-promo-subtitle-directentitlement>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePromoSubtitleDirectentitlement', originStorePromoSubtitle);
}());
