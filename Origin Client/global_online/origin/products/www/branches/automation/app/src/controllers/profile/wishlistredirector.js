/**
 * @see https://confluence.ea.com/pages/viewpage.action?pageId=555199675#Wishlistmodelandpersistence-AccessingawishlistbywishlistID
 */
(function() {
    'use strict';

    function WishlistRedirectorCtrl($scope, $state, $stateParams, AuthFactory, WishlistFactory, NavigationFactory, ScopeHelper) {
        $scope.isLoading = true;

        function notFound() {
            $state.go('app.error_notfound');
        }

        function getWishlistUserId() {
            if(!AuthFactory.isAppLoggedIn()) {
                return;
            }

            return WishlistFactory.getUserIdByWishlistId($stateParams.wishlistId)
                .catch(notFound);
        }

        function redirectToContent(wishlistUserId) {
            $scope.isAuth = AuthFactory.isAppLoggedIn();
            $scope.isLoading = false;

            if ($scope.isAuth && wishlistUserId) {
                NavigationFactory.goUserWishlist(wishlistUserId);
            }

            ScopeHelper.digestIfDigestNotInProgress($scope);
        }

        AuthFactory.waitForAuthReady()
            .then(getWishlistUserId)
            .then(redirectToContent)
            .catch(notFound);
    }

    /**
     * @ngdoc controller
     * @name originApp.controllers:WishlistRedirectorCtrl
     * @description
     *
     * Catch an incoming wishlist ID and forward the user to the correct portal self/friend/visitor
     */
    angular.module('originApp')
        .controller('WishlistRedirectorCtrl', WishlistRedirectorCtrl);
}());
