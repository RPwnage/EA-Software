(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginProfilePageWishlistOtheruserCtrl($scope, WishlistObserverFactory, UtilFactory, WishlistFactory, ObserverFactory, RosterDataFactory) {

        var CONTEXT_NAMESPACE = 'origin-profile-page-wishlist-otheruser';

        $scope.loading = true;

        function updateUser(user) {
            if (!!user) {
                ObserverFactory.create(user.privacy)
                    .defaultTo({isPrivate: true})
                    .attachToScope($scope, 'privacy');

                $scope.emptyWishlistHeadingLoc = UtilFactory.getLocalizedStr($scope.emptywishlistheading, CONTEXT_NAMESPACE, 'emptywishlistheading').replace('%username%', user.originId);
                $scope.emptyWishlistDescriptionLoc = UtilFactory.getLocalizedStr($scope.emptywishlistdescription, CONTEXT_NAMESPACE, 'emptywishlistdescription').replace('%username%', user.originId);

                $scope.loading = false;
            }
        }

        function setWishlist(wishlist) {
            $scope.setWishlist(wishlist);
            if (wishlist.isPrivate) {
                RosterDataFactory.setProfilePrivacy($scope.nucleusId, true);
            }
        }

        RosterDataFactory.getRosterUser($scope.nucleusId).then(updateUser);

        this.setObserver = function() {
            WishlistObserverFactory.getObserver($scope.nucleusId, $scope, UtilFactory.unwrapPromiseAndCall(setWishlist));
        };

    }

    function originProfilePageWishlistOtheruserLink(scope, element, attrs, ctrl) {
        ctrl[0].initializeChildScope(scope);
        scope.loadMore = ctrl[0].loadMore;
        scope.setWishlist = ctrl[0].setWishlist;
        scope.lazyLoadDisabled = ctrl[0].lazyLoadDisabled;
        
        ctrl[1].setObserver();
    }

    function originProfilePageWishlistOtheruser(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            require: ['^originProfilePageWishlist', 'originProfilePageWishlistOtheruser'],
            controller: 'OriginProfilePageWishlistOtheruserCtrl',
            scope: {
                nucleusId: '@nucleusid',
                emptywishlistheading: '@emptywishlistheading',
                emptywishlistdescription: '@emptywishlistdescription'
            },
            link: originProfilePageWishlistOtheruserLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('profile/page/wishlist/views/otheruser.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originProfilePageWishlistOtheruser
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} nucleusid nucleusId of the user
     * @param {LocalizedString} emptywishlistheading message heading to show when a user has not made a wishlist - use %username% to place the user's origin id in the string
     * @param {LocalizedString} emptywishlistdescription message body to show when a user has not made a wishlist - use %username% to place the user's origin id in the string
     * @description
     *
     * Profile Page - Wishlist - Other user
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-profile-page-wishlist-otheruser
     *            nucleusid="123456789"
     *            emptywishlistheading="%username% hasn't started a wishlist yet",
     *            emptywishlistdescription="When %username% creates a wishlist you'll be able to see it here."
     *         ></origin-profile-page-wishlist-otheruser>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginProfilePageWishlistOtheruserCtrl', OriginProfilePageWishlistOtheruserCtrl)
        .directive('originProfilePageWishlistOtheruser', originProfilePageWishlistOtheruser);
}());

