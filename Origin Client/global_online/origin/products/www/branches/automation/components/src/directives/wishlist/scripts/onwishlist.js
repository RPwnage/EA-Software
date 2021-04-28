/**
 * @file wishlist/scripts/onwishlist.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-wishlist-on-wishlist';

    function OriginWishlistOnWishlistCtrl($scope, UtilFactory, WishlistObserverFactory, NavigationFactory) {

        $scope.onWishlistLoc = UtilFactory.getLocalizedStr($scope.onWishlistStr, CONTEXT_NAMESPACE, 'onwishliststr');
        $scope.viewWishlistLoc = UtilFactory.getLocalizedStr($scope.viewWishlistStr, CONTEXT_NAMESPACE, 'viewwishliststr');

        function checkWishlist(wishlist) {
            var inWishlist = _.some(wishlist.offerList, {id: $scope.offerId});

            if (inWishlist !== $scope.onWishlist) {
                $scope.onWishlist = inWishlist;
                $scope.$digest();
            }
        }


        $scope.$watch('offerId', function () {
            WishlistObserverFactory.getObserver(Origin.user.userPid(), $scope, UtilFactory.unwrapPromiseAndCall(checkWishlist));
        });

        $scope.$watch('gameName', function () {
            $scope.onWishlistLoc = (_.isUndefined($scope.gameName) ?
                UtilFactory.getLocalizedStr($scope.onWishlistStr, CONTEXT_NAMESPACE, 'onwishliststr') :
                UtilFactory.getLocalizedStr($scope.gameOnWishlistStr, CONTEXT_NAMESPACE, 'gameonwishliststr').replace('%gamename%', $scope.gameName));
        });

        $scope.goWishlist = function () {
            NavigationFactory.goUserWishlist(Origin.user.userPid());
        };

    }

    function originWishlistOnWishlist(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerId: '@',
                gameName: '@',
                onWishlistStr: '@onwishliststr',
                gameOnWishlistStr: '@gameonwishliststr',
                viewWishlistStr: '@viewwishliststr'
            },
            controller: OriginWishlistOnWishlistCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('wishlist/views/onwishlist.html')
        };
    }



    /**
     * @ngdoc directive
     * @name origin-components.directives:originWishlistOnWishlist
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} onwishliststr "On your wishlist"
     * @param {LocalizedString} gameonwishliststr "%gamename% is on your wishlist"
     * @param {LocalizedString} viewwishliststr "View full wishlist"
     *
     * @description Used on OGD extra content and PDP page to indicate if item is on wishlist
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-wishlist-on-wishlist gameonwishliststr="%gamename% is on your wishlist" onwishliststr="On your wishlist" viewwishliststr="View your wishlist" ></origin-wishlist-on-wishlist>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginWishlistOnWishlistCtrl', OriginWishlistOnWishlistCtrl)
        .directive('originWishlistOnWishlist', originWishlistOnWishlist);
}());
