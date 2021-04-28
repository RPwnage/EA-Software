/**
 * @file cta/scripts/promo.js
 */
(function() {
    'use strict';

    var BUTTON_TYPE = 'primary';

    function originCtaPromo(ComponentsConfigFactory, PurchaseFactory, UtilFactory, InfobubbleFactory) {
        function originCtaPromoLink (scope) {
            scope.type = BUTTON_TYPE;

            function buy() {
                PurchaseFactory.buy(scope.model.offerId, {promoCode: scope.promoCode});
            }

            scope.onBtnClick = function() {
                // Emit an event that is picked up by the tile directive that transcludes this cta,
                // in order to record cta-click telemetry for the tile.
                scope.$emit('cta:clicked');

                if (!UtilFactory.isTouchEnabled()) {
                    buy();
                } else {
                    var infoBubbleParams = {
                        offerId: scope.model.offerId,
                        actionText: scope.btnText,
                        viewDetailsText: scope.viewDetailsText,
                        promoLegalId: scope.promoLegalId
                    };
                    InfobubbleFactory.openDialog(infoBubbleParams, buy);
                }
            };

        }

        return {
            restrict: 'E',
            templateUrl: ComponentsConfigFactory.getTemplatePath('cta/views/cta.html'),
            link: originCtaPromoLink
        };
    }


    /**
     * @ngdoc directive
     * @name origin-components.directives:originCtaPromo
     * @restrict E
     * @element ANY
     * @scope
     * @description Promo cta opens checkout for a given offer with a given promo code
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-cta-promo btnText="Buy" promoCode="3TB3-3VEU-866M-5844-M7XB"></origin-cta-promo>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originCtaPromo',  originCtaPromo);
}());