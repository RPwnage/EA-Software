/**
 * @file store/bundle/offer.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-bundle';

    function OriginStoreBundleOfferCtrl($scope, $location, $filter, UtilFactory, ProductFactory, StoreBundleFactory, PdpUrlFactory) {

        $scope.strings = {
            addText: UtilFactory.getLocalizedStr($scope.addText, CONTEXT_NAMESPACE, 'addstr'),
            inBundleText: UtilFactory.getLocalizedStr($scope.inBundleText, CONTEXT_NAMESPACE, 'inbundlestr'),
            learnMoreText: UtilFactory.getLocalizedStr($scope.learnMoreText, CONTEXT_NAMESPACE, 'learnmorestr')
        };

        $scope.infobubbleContent = UtilFactory.getLocalizedStr($scope.infobubbleContent, CONTEXT_NAMESPACE, 'infobubblecontentstr');

        $scope.addToBundle = function($event) {
            $event.stopPropagation();
            StoreBundleFactory.add($scope.offerId);
            $scope.$broadcast('infobubble-remove');
        };

        $scope.goToPdp = PdpUrlFactory.goToPdp;

        /**
         * Checks whether the current offer is in the active bundle or not
         * @return {boolean}
         */
        $scope.isInBundle = function() {
            return StoreBundleFactory.has($scope.offerId);
        };
        /**
         * Checks whether the user owns the current offer or not
         * @return {boolean}
         */
        $scope.isOwned = function() {
            return $scope.model.isOwned;
        };

        $scope.model = {};
        var removeWatcher = $scope.$watch('offerId', function (newValue) {
            if (newValue) {
                removeWatcher();
                ProductFactory.get(newValue).attachToScope($scope, 'model');
            }
        });
    }

    function originStoreBundleOffer(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                offerId: '@',
                addText: '@',
                inBundleText: '@'
            },
            controller: 'OriginStoreBundleOfferCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/bundle/views/offer.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreBundleOffer
     * @restrict E
     * @element ANY
     * @scope
     * @description PDP OTH Details block
     * @param {OfferId} offer-id the offer id
     * @param {LocalizedString|OCD} add-text The CTA text
     * @param {LocalizedString|OCD} in-bundle-text The "Item Added" text
     *
     * @example
     * <origin-store-row>
     *     <origin-store-bundle-offer
     *      add-text="Add to Bundle"
     *      in-bundle-text="Item Added"
     *     ></origin-store-bundle-offer>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .directive('originStoreBundleOffer', originStoreBundleOffer)
        .controller('OriginStoreBundleOfferCtrl', OriginStoreBundleOfferCtrl);
}());
