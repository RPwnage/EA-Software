
/** 
 * @file store/game/scripts/wishlist.js
 */ 
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-game-wishlist';

    function OriginStoreGameWishlistCtrl($scope, AuthFactory, WishlistObserverFactory, UtilFactory, WishlistFactory, DirectiveScope) {
        var wishlist = WishlistFactory.getWishlist(Origin.user.userPid());

        $scope.isWishlistEnabled = WishlistFactory.isWishlistEnabled();
        $scope.isLoaded = false;

        function updateWishlist() {
            if($scope.onWishlist) {
                wishlist.removeOffer($scope.offerId);
                Origin.telemetry.sendTelemetryEvent('TRACKER_MARKETING', 'Wishlist', 'Remove', $scope.offerId);
            } else {
                wishlist.addOffer($scope.offerId);
                Origin.telemetry.sendTelemetryEvent('TRACKER_MARKETING', 'Wishlist', 'Add', $scope.offerId);
            }
        }

        function setIsLoaded() {
             $scope.isLoaded = true;
        }

        $scope.updateWishlist = function($event) {
            $event.stopPropagation();
            $scope.isLoaded = false;
            AuthFactory
                .promiseLogin()
                .then(updateWishlist)
                .catch(setIsLoaded);
        };

        function checkWishlist(wishlist) {
            var inWishlist = _.some(wishlist.offerList, {id: $scope.offerId});

            if (inWishlist !== $scope.onWishlist) {
                $scope.onWishlist = inWishlist;
                $scope.$digest();
            }
        }

        $scope.$watch('onWishlist', setIsLoaded);

        WishlistObserverFactory.getObserver(Origin.user.userPid(), $scope, UtilFactory.unwrapPromiseAndCall(checkWishlist));

        DirectiveScope.populateScope($scope, CONTEXT_NAMESPACE);
    }

    function originStoreGameWishlist(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerId: '@',
                add: '@',
                remove: '@'
            },
            controller: OriginStoreGameWishlistCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/game/views/wishlist.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreGameWishlist
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {String} offer-id the offer id
     * @param {LocalizedString} add The add to wishlist text
     * @param {LocalizedString} remove The remove from wishlist text
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-game-wishlist />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreGameWishlist', originStoreGameWishlist);
}()); 
