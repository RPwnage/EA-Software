
/**
 * @file store/promo/scripts/offerdetails-directentitlement.js
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

    var CONTEXT_NAMESPACE = 'origin-store-promo-offerdetails-directentitlement';

    function originStorePromoOfferdetailsDirectentitlement(ComponentsConfigFactory, DirectiveScope) {

        function originStorePromoOfferdetailsDirectentitlementLink (scope, element, attributes, controller) {
            scope.isEntitlement = true;

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
                videoid : '@',
                matureContent: '@',
                ocdPath: '@',
                showPackart: '@',
                bulletOne: '@',
                bulletTwo: '@',
                backgroundColor: '@',
                badgeImage: '@',
                ratingFontColor: '@',
                useH1Tag: '@',
                othText: '@',
                titleImage: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/promo/views/offerdetails.html'),
            controller: 'originStorePromoOfferdetailsCtrl',
            link: originStorePromoOfferdetailsDirectentitlementLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePromoOfferdetailsDirectentitlement
     * @restrict E
     * @element ANY
     * @description Merchandizes a promo module with offer details as its content.
     * @param {LayoutEnumeration} layout The type of layout this promo will have.
     * @param {ImageUrl} image The background image for this module.
     * @param {ImageUrl} badge-image optional badge image. Only applicable to hero layout
     * @param {LocalizedString} title-str Promo title
     * @param {Video} videoid The youtube video ID of the video for this promo (Optional).
     * @param {BooleanEnumeration} mature-content Whether the video(s) defined within this component should be considered Mature
     * @param {string} background-color Hex value of the background color
     * @param {OcdPath} ocd-path OCD Path
     * @param {BooleanEnumeration} show-packart Determines whether the pack art from ocdpath will be displayed
     * @param {LocalizedTemplateString} bullet-one First bullet point (Optional)
     * @param {LocalizedTemplateString} bullet-two Second bullet point (Optional)
     * @param {RatingEnumeration} rating-font-color color of the ratings font (TW only)
     * @param {String} ctid Content tracking ID
     * @param {BooleanEnumeration} use-h1-tag use H1 Tag for the title to boost SEO relevance. Only one H1 should be present on a page. Optional and defaults to H2
     * @param {LocalizedString} oth-text * on the house text.
     * @param {ImageUrl} title-image title image. Will be used only if title str is not merchandised
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-promo-offerdetails-directentitlement
     *         layout="hero"
     *         image="https://docs.x.origin.com/proto/images/store/wide-ggg.jpg"
     *         title-str="We Stand By Our Games."
     *         videoid="wS7gfJqFoA">
     *     </origin-store-promo-offerdetails-directentitlement>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePromoOfferdetailsDirectentitlement', originStorePromoOfferdetailsDirectentitlement);
}());
