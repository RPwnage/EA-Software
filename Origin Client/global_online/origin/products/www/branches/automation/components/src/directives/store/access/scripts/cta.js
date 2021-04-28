
/** 
 * @file store/access/scripts/cta.js
 */ 
(function(){
    'use strict';

    function OriginStoreAccessCtaCtrl($scope, AuthFactory, PurchaseFactory, ErrorModalFactory) {
        /**
         * Clickity-click on the OAX CTA button
         */
        $scope.btnClick = function() {
            AuthFactory.promiseLogin()
                .then(_.partial(PurchaseFactory.verifyTrialStatus, $scope.offerId))
                .then(_.partial(PurchaseFactory.subscriptionCheckout, $scope.offerId))
                .catch(ErrorModalFactory.showDialog);
        };
    }

    function originStoreAccessCta(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                button: '@',
                offerId: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/access/views/cta.html'),
            controller: OriginStoreAccessCtaCtrl
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAccessCta
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} button Text for the purchase CTA button
     * @param {OfferId} offer-id subscription entitlement offerId
     * @description
     *
     * CTA button directive that triggers the subscriptions checkout flow. Uses
     * offerId because subscription offers cannot use OCD path.
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *      <origin-store-access-cta
     *          button="Click Me"
     *          offer-id="Origin.OFR.50.0001171">
     *      </<origin-store-access-cta>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreAccessCta', originStoreAccessCta);
}()); 
