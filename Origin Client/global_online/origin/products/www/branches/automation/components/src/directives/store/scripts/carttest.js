
/**
 * @file /store/scripts/carttest.js
 */
(function(){
    'use strict';
    function OriginStoreCarttestCtrl($scope, CartModel, GamesDataFactory, ErrorModalFactory, SubscriptionFactory, GamesTrialFactory) {
        var OFFER_GOOD = 'OFB-EAST:109549060'; // BF4-DDX
        var OFFER_BAD = 'Origin.OFR.50.0001012';
        var COUPON_CODE_10_OFF = 'HEV5-XG48-2FUH-HWFE-7HH3';
        var COUPON_CODE_15_OFF = '8Y8Y-LBWT-YP9F-LGFC-BUCE';
        var VAULT_OFFER_ID = 'Origin.OFR.50.0000462'; // Fifa 15

        function out(object) {
            window.alert(JSON.stringify(object, null, 2));
        }

        function errOut(object) {
            window.alert('An error occurred, resulting in the following response:\n\n'+JSON.stringify(object, null, 2));
        }

        $scope.getCart = function() {
            CartModel.getCart()
                .then(out)
                .catch(errOut);
        };

        $scope.addOffer = function() {
            CartModel.addOffer(OFFER_GOOD)
                .then(out)
                .catch(errOut);
        };

        $scope.addOfferNoClear = function() {
            CartModel.addOffer(OFFER_GOOD, {needClearCart: false})
                .then(out)
                .catch(errOut);
        };

        $scope.removeOffer = function() {
            CartModel.removeOfferByOfferId(OFFER_GOOD)
                .then(out)
                .catch(errOut);
        };

        $scope.removeOtherOffer = function() {
            CartModel.removeOfferByOfferId(OFFER_BAD)
                .then(out)
                .catch(errOut);
        };

        $scope.addCoupon = function(){
            CartModel.addCoupon(COUPON_CODE_10_OFF)
                .then(out)
                .catch(errOut);
        };

        $scope.removeCoupon = function() {
            CartModel.removeCouponByCode(COUPON_CODE_10_OFF)
                .then(out)
                .catch(errOut);
        };

        $scope.removeOtherCoupon = function() {
            CartModel.removeCouponByCode(COUPON_CODE_15_OFF)
                .then(out)
                .catch(errOut);
        };

        $scope.clearCart = function() {
            CartModel.clearCart()
                .then(function(){
                    CartModel.getCart().then(out);
                })
                .catch(errOut);
        };

        $scope.gimmeFreeGame = function(offerId) {
            GamesDataFactory.directEntitle(offerId)
                .then(out)
                .catch(errOut);
        };

        $scope.showError = function(errorCode) {
            ErrorModalFactory.showDialog(errorCode);
        };

        /**
         * Add or remove vault game entitlement
         * @param  {string} type entitle or remove
         */
        $scope.vaultGame = function(type) {
            var offerId = window.prompt("Enter vault offerId", VAULT_OFFER_ID);
            if (type === 'entitle') {
                SubscriptionFactory.vaultEntitle(offerId)
                    .then(out)
                    .catch(errOut);
            } else {
                GamesDataFactory.vaultRemove(offerId)
                    .then(out)
                    .catch(errOut);
            }
        };

        $scope.checkTrialTime = function(contentId) {
            SubscriptionFactory.getTrialTime(contentId)
                .then(out)
                .catch(errOut);
        };

        function handleGetContentIdResponse(contentId) {
            if (!contentId) {
                return Promise.reject({success: false, message: 'content-id-not-found'});
            } else {
                return GamesTrialFactory.getTrialTime(contentId);
            }
        }
        $scope.checkTrialTimeByOfferId = function(offerId) {
            GamesDataFactory.getContentId(offerId)
                .then(handleGetContentIdResponse)
                .then(out)
                .catch(errOut);
        };
    }

    function originStoreCarttest(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {},
            controller: 'OriginStoreCarttestCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/carttest.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreCarttest
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin--store-carttest />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreCarttestCtrl', OriginStoreCarttestCtrl)
        .directive('originStoreCarttest', originStoreCarttest);
}());
