(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginProfilePageWishlistCtrl($scope, $interval, ObjectHelperFactory) {
        var childScope;
        var PAGE_SIZE = 20,
            LAZY_LOAD_INTERVAL = 2000,
            LAZY_LOAD_DELAY = 1000,
            SPINNER_DIGEST_DELAY = 2000,
            lazyLoadInterval,
            orderByField = ObjectHelperFactory.orderByField;


        $scope.isSelf = ($scope.nucleusId === '') || (Number($scope.nucleusId) === Number(Origin.user.userPid()));

        function init() {
            childScope.lazyLoadOfferList = [];
            $interval.cancel(lazyLoadInterval);
        }

        function onDestroy() {
            init();
        }

        this.lazyLoadDisabled = function() {
            if (_.isArray(childScope.offerList)) {
                return ( childScope.lazyLoadOfferList.length === childScope.offerList.length);
            }
            return true;
        };

        this.initializeChildScope = function(scope) {
            childScope = scope;
            init();

            childScope.$on('$destroy', onDestroy);
        };

        this.setWishlist= function(wishlist) {
          childScope.wishlist = wishlist;
          childScope.offerList = orderByField('ts')(wishlist.offerList);
          childScope.lazyLoadOfferList = _.slice(childScope.offerList, 0, PAGE_SIZE);
          childScope.$digest();
        };

        function loadMoreItems() {
            if (_.isArray(childScope.offerList)) {
                if (childScope.lazyLoadOfferList.length < childScope.offerList.length) {
                    childScope.lazyLoadOfferList = _.slice(childScope.offerList, 0, childScope.lazyLoadOfferList.length + PAGE_SIZE);
                } else {
                    $interval.cancel(lazyLoadInterval);
                }
            }
        }

        this.loadMoreItems = loadMoreItems;
        
        setTimeout(function() {
            lazyLoadInterval = $interval(loadMoreItems, LAZY_LOAD_INTERVAL);
        }, LAZY_LOAD_DELAY);

        /**
         * Load more results.
         * @returns {Promise}
         */
        this.loadMore = function () {
            return new Promise(function (resolve) {
                loadMoreItems();
                var unhook = childScope.$on('postrepeat:last', function() {
                    unhook();
                    resolve();
                });            
                setTimeout(function() {childScope.$digest();}, SPINNER_DIGEST_DELAY);
            });

        };

    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originProfilePageWishlist
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} nucleusid nucleusId of the user
     * @description
     *
     * Profile Page - Wishlist
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-profile-page-wishlist nucleusid="123456789"
     *         ></origin-profile-page-wishlist>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginProfilePageWishlistCtrl', OriginProfilePageWishlistCtrl)
        .directive('originProfilePageWishlist', function (ComponentsConfigFactory) {

            return {
                restrict: 'E',
                controller: 'OriginProfilePageWishlistCtrl',
                scope: {
                    nucleusId: '@nucleusid'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('profile/page/views/wishlist.html')
            };

        });
}());

