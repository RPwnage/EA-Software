
/**
 * @file store/promo/scripts/cta.js
 */
(function(){
    'use strict';

    /* jshint ignore:start */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    function originStorePromoCta(ComponentsConfigFactory, AnimateFactory, ScreenFactory, ProductFactory) {

        function originStorePromoCtaLink(scope){

            function adjustSize(){
                scope.largeScreen = ScreenFactory.isPdpLarge();
            }

            function adjustSizeDigest(){
                adjustSize();
                scope.$digest();
            }

            scope.getButtonType = function() {
                return scope.largeScreen ? 'primary': 'link';
            };

            scope.$watchOnce('offerId', function () {
                ProductFactory.get(scope.offerId).attachToScope(scope, 'model');
            });

            scope.infobubbleContent =  '<origin-store-game-rating></origin-store-game-rating><origin-store-game-legal></origin-store-game-legal>';

            AnimateFactory.addResizeEventHandler(scope, adjustSizeDigest, 100);
            adjustSize();
        }

        return {
            restrict: 'E',
            scope: {
                ctaText: '@',
                href: '@',
                isEntitlement: '@',
                offerId: '@'
            },
            link: originStorePromoCtaLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/promo/views/cta.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePromoCta
     * @restrict E
     * @element ANY
     * @description Makes a CTA link/button that is intended to live inside a
     * promo banner. This CTA is an inline link at < 'large desktop size' and turns into a
     * cta-secondary transparent button aligned right at > 'large desktop size'.
     * @param {LocalizedString} cta-text Text for the link
     * @param {BooleanEnumeration} is-entitlement true if this CTA is a direct entitlement, otherwise, it will be a link to PDP
     * @param {OfferId} offer-id desc 
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *       <h3 class="origin-store-promo-banner-subtitle otktitle-3">{{ ::text }}<origin-store-promo-cta cta-text=""></origin-store-promo-cta></h3>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePromoCta', originStorePromoCta);
}());
