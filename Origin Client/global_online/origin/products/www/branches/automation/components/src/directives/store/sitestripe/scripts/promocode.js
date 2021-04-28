
/** 
 * @file store/sitestripe/scripts/promocode.js
 */ 
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-sitestripe-promocode';
    var FontColorEnumeration = {
        "light" : "#FFFFFF",
        "dark" : "#1E262C"
    };
    var FONT_CLASS_DARK = 'origin-store-sitestripe-promocode-color-dark';
    var FONT_CLASS_LIGHT = 'origin-store-sitestripe-promocode-color-light';

    function OriginStoreSitestripePromocodeCtrl($scope, NavigationFactory, CSSUtilFactory) {
        $scope.goTo = function(event, href){
            event.preventDefault();
            NavigationFactory.openLink(href);
        };
        $scope.fontColorClass = $scope.fontcolor === FontColorEnumeration.dark ? FONT_CLASS_DARK : FONT_CLASS_LIGHT;

        $scope.promoBorderColor = CSSUtilFactory.hexToRgba($scope.borderColor);

        $scope.promoBackgroundColor = CSSUtilFactory.hexToRgba($scope.accentcolor);

    }

    function originStoreSitestripePromocode(ComponentsConfigFactory, SiteStripeFactory) {
        return {
            restrict: 'E',
            scope: {
                uniqueSitestripeId: '@',
                image: '@',
                backgroundColor: '@',
                fontcolor: '@',
                promoBackgroundColor: '@',
                promoBorderColor: '@',
                savingsText: '@',
                prePromoCodeText: '@',
                promoCodeText: '@',
                postPromoCodeText: '@',
                href: '@',
                ctid: '@',
                legalDisclaimerText: '@',
                legalDisclaimerLinkText: '@',
                legalDisclaimerDialogHeaderText: '@',
                legalDisclaimerDialogBodyTitleText: '@',
                legalDisclaimerDialogBodyText: '@',
                showOnPage: '@',
                hideOnPage: '@'
            },
            controller: OriginStoreSitestripePromocodeCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/sitestripe/views/promocode.html'),
            link: SiteStripeFactory.siteStripeLinkFn(CONTEXT_NAMESPACE)
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreSitestripePromocode
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} savings-text To format this sitestripe: follow this pattern to get the following example text: "30% off Use GIFTOFPLAY at checkout". "30% off" {savingsText}, "Use" {pre-promo-code-text}, "GIFTOFPLAY" {promo-code-text}, "at checkout" {post-promo-code-text} Max character count of 30 for best fit.
     * @param {LocalizedString} pre-promo-code-text Joins savings text and the promo code together.
     * @param {LocalizedString} promo-code-text the promo code
     * @param {LocalizedString} post-promo-code-text the "at checkout" string
     * @param {ImageUrl} image The background image
     * @param {String} background-color The background color for this site stripe (Hex format #000000).
     * @param {FontColorEnumeration} fontcolor The font color of the site stripe, light or dark.
     * @param {String} promo-background-color The background color for promo code (Hex format #000000).
     * @param {String} promo-border-color The border color for promo code (Hex format #000000).
     * @param {String} href the link URL (optional)
     * @param {string} unique-sitestripe-id a unique id to identify this site stripe from others. Must be specified to enable site stripe dismissal.
     * @param {string} ctid Campaign Targeting ID
     * @param {LocalizedString} legal-disclaimer-text legal claimer text
     * @param {LocalizedString} legal-disclaimer-link-text - legal disclaimer link text. Will be replaced into placeholder %legal-disclaimer-link-text% in legal-disclaimer-text
     * @param {LocalizedString} legal-disclaimer-dialog-header-text legal disclaimer dialog header text.
     * @param {LocalizedString} legal-disclaimer-dialog-body-title-text legal disclaimer dialog body title
     * @param {LocalizedString} legal-disclaimer-dialog-body-text legal disclaimer dialog body text
     * @param {string} show-on-page Urls to show this sitestripe on (comma separated)
     * @param {string} hide-on-page Urls to hide this sitestripe on (comma separated)
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-sitestripe-promocode />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreSitestripePromocode', originStoreSitestripePromocode);
}()); 
