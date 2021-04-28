/**
 * @file cta/scripts/gift.js
 */
(function () {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-cta-gift';


    function originCtaGift(ComponentsConfigFactory, UtilFactory, GiftingWizardFactory, GiftingFactory) {

        function originGiftCtaLink(scope) {
            scope.strings = {};
            var modelPromise;


            scope.buyAsGift = function () {
                modelPromise.then(function () {
                    if (scope.nucleusId) {
                        GiftingWizardFactory.loginAndStartCustomizeMessageFlow(scope.model, {nucleusId : scope.nucleusId});
                    } else {
                        GiftingWizardFactory.loginAndStartGiftingFlow(scope.model);
                    }
                });
            };

            function localize() {
                var formattedPrice = _.get(scope, ['model', 'formattedPrice']);

                if(formattedPrice && formattedPrice !== 'undefined') {
                    scope.strings.buyAsGift = UtilFactory.getLocalizedStr((scope.buyAsGiftStr), CONTEXT_NAMESPACE, 'buy-as-gift-str', {'%price%': formattedPrice});
                } else {
                    scope.strings.buyAsGift = UtilFactory.getLocalizedStr((scope.buyAsGiftStr), CONTEXT_NAMESPACE, 'buy-as-gift-str', {'%price%': ''});
                }
            }

            /**
             * Two use cases
             *
             * if a nucleusId is provided, Don't need to check whether the game is purchasable (for wishlist)
             * if no nucleusId is provided, then check whether the game is purchasable
             */
            function checkGiftability(model) {
                scope.isGiftable = GiftingFactory.isGameGiftable(model);
            }

            modelPromise = new Promise(function (resolve) {
                scope.$watchCollection('model', function (newValue) {
                    if (newValue && !_.isEmpty(newValue)) {
                        localize();
                        checkGiftability(newValue);
                        resolve();
                    }
                });
            });


        }

        return {
            restrict: 'E',
            scope: {
                buyAsGiftStr: '@',
                nucleusId: '@',
                model: '=',
                type: '@'
            },
            link: originGiftCtaLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('cta/views/giftcta.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originCtaGift
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * @param {string} type type of CTA
     * @param {Object} model model object
     * @param {Number} nucleus-id id of the user to gift to
     * @param {LocalizedString} buy-as-gift-str Buy As Gift text
     * @param {LocalizedString} gift-game-str Gift this game text (note: consumed by purchase.js)
     * @param {LocalizedString} add-wishlist-game-str Add to wishlist context menu cta text (note: consumed by purchase.js)
     * @param {LocalizedString} remove-wishlist-game-str Remove from wishlist context menu cta text (note: consumed by purchase.js)
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-cta-gift model="" type="transparent"></origin-cta-gift>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .directive('originCtaGift', originCtaGift);
}());