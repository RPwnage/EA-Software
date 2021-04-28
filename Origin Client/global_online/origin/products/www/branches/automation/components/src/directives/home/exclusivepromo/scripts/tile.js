/**
 * @file home/exclusivepromo/tile.js
 */
(function() {
    'use strict';

    var INFOBUBBLE_NAMESPACE = 'origin-dialog-content-mobile-infobubble';
    var HOVER_CLASS = 'origin-home-exclusive-promo-hover';
    var CONTEXT_NAMESPACE = 'origin-home-exclusivepromo-tile';

    function originHomeExclusivepromoTile(ComponentsConfigFactory, OcdPathFactory, UtilFactory, InfobubbleFactory, PurchaseFactory, PriceInsertionFactory, AppCommFactory) {
        function originHomeExclusivepromoTileLink(scope, element) {
            scope.strings = {};
            function buy() {
                PurchaseFactory.buy(scope.model.offerId, {promoCode: scope.promoCode});
            }

            scope.$watchOnce('model', function () {
                scope.infobubbleContent =  '<origin-store-game-rating></origin-store-game-rating>'+
                    '<origin-store-game-legal></origin-store-game-legal>';
            });

            scope.$watch('model.isOwned', function(isOwned, oldValue){
                if (isOwned && isOwned !== oldValue) {
                    element.remove();
                    AppCommFactory.events.fire('origin-route:reloadcurrentRoute');
                }
            });

            scope.openMobileInfobubble = function () {
                var infobubbleParams = {
                    offerId: scope.model.offerId,
                    actionText: scope.btnText,
                    viewDetailsText: scope.viewDetailsText,
                    promoLegalId: scope.promoLegalId
                };

                if (UtilFactory.isTouchEnabled()) {
                    InfobubbleFactory.openDialog(infobubbleParams, buy);
                }
            };

            scope.viewDetailsText = UtilFactory.getLocalizedStr(scope.viewDetailsText, INFOBUBBLE_NAMESPACE, 'view-details-text');

            function buildCalloutHtml(calloutText) {
                return calloutText ? '<span class="callout-text">' + calloutText + '</span>' : '';
            }

            function retrievePrice(field){
                var replacementObject = {
                    '%callout-text%': buildCalloutHtml(scope.calloutText)
                };

                if (scope.useDynamicPriceCallout){
                    PriceInsertionFactory.insertPriceIntoLocalizedStr(scope, scope.strings, scope[field], CONTEXT_NAMESPACE, field, replacementObject);
                }else if (scope.useDynamicDiscountPriceCallout){
                    PriceInsertionFactory
                        .insertDiscountedPriceIntoLocalizedStr(scope, scope.strings, scope[field], CONTEXT_NAMESPACE, field, replacementObject);
                }else if (scope.useDynamicDiscountRateCallout){
                    PriceInsertionFactory
                        .insertDiscountedPercentageIntoLocalizedStr(scope, scope.strings, scope[field], CONTEXT_NAMESPACE, field, replacementObject);
                }else{
                    PriceInsertionFactory
                        .insertPriceIntoLocalizedStr(scope, scope.strings, scope[field], CONTEXT_NAMESPACE, field, replacementObject);
                }
            }

            retrievePrice('text');
            retrievePrice('headline');

            OcdPathFactory.get(scope.ocdPath).attachToScope(scope, 'model');

            /* This adds a hover class when the infobubble is hovered */
            scope.$on('infobubble-show', function() {
                element.addClass(HOVER_CLASS);
            });

            /* this removes the class */
            scope.$on('infobubble-hide', function() {
                element.removeClass(HOVER_CLASS);
            });
        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
                image: '@',
                headline: '@',
                text: '@',
                promoCode: '@',
                promoLegalId: '@',
                ocdPath: '@',
                btnText: '@',
                calloutText: '@',
                useDynamicPriceCallout: '@',
                useDynamicDiscountPriceCallout: '@',
                useDynamicDiscountRateCallout: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/exclusivepromo/views/tile.html'),
            link: originHomeExclusivepromoTileLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeExclusivepromoTile
     * @restrict E
     * @element ANY
     * @scope
     * @param {ImageUrl} image Image in the background
     * @param {LocalizedString} headline Title text
     * @param {LocalizedString} text Description text
     * @param {String} promo-code Promo code to be used
     * @param {String} promo-legal-id Id that is used for the promotion on the terms and conditions page. This will correspond to the 'id' field of the origin-policyitem that is merch'd for this promotion.
     * @param {OcdPath} ocd-path Ocd path of the offer to be bought
     * @param {LocalizedString} btn-text Text for the purchase promo button
     * @param {String} ctid the Content Tracking ID used for telemetry
     * @param {LocalizedString} callout-text text that will be replaced into placeholder %callout-text% in headline or text
     *
     * @description Exclusive Promo tile
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-exclusivepromo-tile></origin-home-exclusivepromo-tile>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originHomeExclusivepromoTile', originHomeExclusivepromoTile);

}());