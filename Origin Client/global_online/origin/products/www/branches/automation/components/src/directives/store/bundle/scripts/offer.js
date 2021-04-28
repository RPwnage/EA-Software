/**
 * @file store/bundle/offer.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-bundle-offer',
        THROTTLE_DELAY_MS = 500;

    function OriginStoreBundleOfferCtrl($scope, UtilFactory, OcdPathFactory, StoreBundleFactory, PdpUrlFactory, InfobubbleFactory, AppCommFactory, ScreenFactory, AnimateFactory) {
        var isCompactView = false;
        $scope.isTouchEnabled = UtilFactory.isTouchEnabled();
        
        $scope.strings = {
            addText: UtilFactory.getLocalizedStr($scope.addText, CONTEXT_NAMESPACE, 'add-text'),
            inBundleText: UtilFactory.getLocalizedStr($scope.inBundleText, CONTEXT_NAMESPACE, 'in-bundle-text'),
            learnMoreText: UtilFactory.getLocalizedStr($scope.learnMoreText, CONTEXT_NAMESPACE, 'learn-more-text'),
            viewDetailsText: UtilFactory.getLocalizedStr($scope.viewDetailsText, CONTEXT_NAMESPACE, 'view-details-text'),
            removeText: UtilFactory.getLocalizedStr($scope.removeText, CONTEXT_NAMESPACE, 'remove-text')
        };

        $scope.infobubbleContent = '<origin-store-game-rating></origin-store-game-rating><origin-store-game-legal></origin-store-game-legal>';

        $scope.addToBundle = function($event) {
            $event.stopPropagation();
            $scope.$broadcast('infobubble-remove');
            addToBundle($scope.model.offerId);
        };

        function reSetupCarouselCart(){
            window.requestAnimationFrame(function(){
                AppCommFactory.events.fire('carousel:resetup');
            });
        }

        function addToBundle(offerId) {
            StoreBundleFactory.add(offerId);
            reSetupCarouselCart();
        }

        $scope.openMobileLink = function(){
            if ($scope.isTouchEnabled){
                if (!$scope.isOwned()) {
                    var params = {
                        offerId: $scope.model.offerId,
                        actionText: $scope.strings.addText,
                        viewDetailsText: $scope.strings.viewDetailsText
                    };
                    InfobubbleFactory.openDialog(params, _.partial(addToBundle, $scope.model.offerId));
                } else {
                    $scope.goToPdp($scope.model);
                }
            }
        };

       $scope.isNotInBundleAndNotOwned = function() {
           return !$scope.isInBundle() && !$scope.isOwned() && !$scope.isTouchEnabled;
       };

        $scope.isNotInBundleAndOwned = function(){
            return $scope.isOwned() && !$scope.isInBundle() && !$scope.isTouchEnabled;
        };

        $scope.isInBundleAndShowRemoveLink = function(){
            return $scope.isInBundle() && (isCompactView || $scope.isTouchEnabled);
        };

        $scope.goToPdp = function(model, $event){
            $event.preventDefault();
            PdpUrlFactory.goToPdp(model);
        };

        $scope.goToPdpIfNotMobile = function(model, $event){
            $event.preventDefault();
            if (!$scope.isTouchEnabled){
                PdpUrlFactory.goToPdp(model);
            }
        };

        $scope.getPdpUrl = PdpUrlFactory.getPdpUrl;

        /**
         * Checks whether the current offer is in the active bundle or not
         * @return {boolean}
         */
        $scope.isInBundle = function() {
            return StoreBundleFactory.has($scope.model.offerId);
        };
        /**
         * Checks whether the user owns the current offer or not
         * @return {boolean}
         */
        $scope.isOwned = function() {
            return $scope.model.isOwned;
        };

        /**
         * Remove offer from bundle cart
         * @param offerIdc
         */
        $scope.removeOffer = function(){
            StoreBundleFactory.remove($scope.model.offerId);
            reSetupCarouselCart();
        };

        $scope.model = {};
        var removeWatcher = $scope.$watch('ocdPath', function (newValue) {
            if (newValue) {
                removeWatcher();
                OcdPathFactory.get(newValue).attachToScope($scope, 'model');
            }
        });

        function updateViewSize(){
            isCompactView = ScreenFactory.isSmall();
        }

        function updateViewSizeDigest(){
            updateViewSize();
            $scope.$digest();
        }

        AnimateFactory.addResizeEventHandler($scope, updateViewSizeDigest, THROTTLE_DELAY_MS);


        updateViewSize();
    }

    function originStoreBundleOffer(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                ocdPath: '@',
                addText: '@',
                inBundleText: '@',
                viewDetailsText: '@',
                removeText: '@'
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
     * @param {OcdPath} ocd-path the ocd path
     * @param {LocalizedString|OCD} add-text The CTA text
     * @param {LocalizedString|OCD} in-bundle-text The "Item Added" text
     * @param {LocalizedString} learn-more-text learn more text.
     * @param {LocalizedString} view-details-text view details button text. For mobile add cart item dialog.
     * @param {LocalizedString} remove-text remove link text. For mobile view to remove cart item.
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
