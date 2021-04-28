(function () {

    'use strict';

    var CONTEXT_NAMESPACE = 'origin-profile-page-wishlist-self';

    /**
    * The controller
    */
    function OriginProfilePageWishlistSelfCtrl($scope, $element, $compile, UtilFactory, ComponentsConfigHelper, NavigationFactory, WishlistObserverFactory, DialogFactory, WishlistFactory, ScopeHelper) {
        $scope.emptyWishlistHeadingLoc = UtilFactory.getLocalizedStr($scope.emptywishlistheading, CONTEXT_NAMESPACE, 'emptywishlistheading');
        $scope.emptyWishlistDescriptionLoc = UtilFactory.getLocalizedStr($scope.emptywishlistdescription, CONTEXT_NAMESPACE, 'emptywishlistdescription');
        $scope.privateProfileHeadingLoc = UtilFactory.getLocalizedStr($scope.privateprofileheading, CONTEXT_NAMESPACE, 'privateprofileheading');
        $scope.shareCtaLoc = UtilFactory.getLocalizedStr($scope.sharecta, CONTEXT_NAMESPACE, 'sharecta');
        $scope.addToWishlistImageSrc = UtilFactory.getLocalizedStr($scope.addtowishlistimage, CONTEXT_NAMESPACE, 'addtowishlistimage');
        $scope.shareYourWishlistLoc = UtilFactory.getLocalizedStr($scope.shareyourwishlist, CONTEXT_NAMESPACE, 'shareyourwishlist');
        $scope.userId = Origin.user.userPid();
        $scope.isPrivate = (Origin.user.showPersona() === Origin.defines.showPersona.NO_ONE);

        function updateWishlistLink(wishlistId) {

            $scope.showShareWishlist = !_.isUndefined(wishlistId);

            $scope.openShareWishlist = function() {

                DialogFactory.openDirective({
                    id: 'origin-dialog-content-sharewishlist',
                    name: 'origin-dialog-content-sharewishlist',
                    size: 'large',
                    data: {
                        wishlistid: wishlistId
                    }
                });
            };

            ScopeHelper.digestIfDigestNotInProgress($scope);
        }

        function appendPrivateProfileDescription() {
            var descriptionLoc = UtilFactory.getLocalizedStr($scope.privateprofiledescription, CONTEXT_NAMESPACE, 'privateprofiledescription'),
                descriptionContainer = $element.find('.origin-profile-wishlist-private-body-text'),
                descriptionContent = $compile(['<span>', descriptionLoc, '</span>'].join(''))($scope);

            descriptionContainer.append(descriptionContent);

            ScopeHelper.digestIfDigestNotInProgress($scope);
        }

        $scope.goToAccount = function($event) {
            $event.stopPropagation();
            $event.preventDefault();

            var accountsUrl = ComponentsConfigHelper.getUrl('accountPrivacySettings')
                .replace("{locale}", Origin.locale.locale());

            NavigationFactory.externalUrl(accountsUrl, true);
        };



        this.setObserver = function() {
            WishlistObserverFactory.getObserver($scope.userId, $scope, UtilFactory.unwrapPromiseAndCall($scope.setWishlist));
        };

        setTimeout(appendPrivateProfileDescription, 0);

        WishlistFactory.getWishlist($scope.userId).getWishlistId()
            .then(updateWishlistLink);
    }

    function originProfilePageWishlistSelfLink(scope, element, attrs, ctrl) {
        ctrl[0].initializeChildScope(scope);
        scope.loadMore = ctrl[0].loadMore;
        scope.setWishlist = ctrl[0].setWishlist;
        scope.lazyLoadDisabled = ctrl[0].lazyLoadDisabled;

        ctrl[1].setObserver();
    }

    function originProfilePageWishlistSelf(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            require: ['^originProfilePageWishlist', 'originProfilePageWishlistSelf'],
            controller: OriginProfilePageWishlistSelfCtrl,
            scope: {
                emptywishlistheading: '@',
                emptywishlistdescription: '@',
                privateprofileheading: '@',
                privateprofiledescription: '@',
                sharecta: '@',
                addtowishlistimage: '@',
                shareyourwishlist: '@'
            },
            link: originProfilePageWishlistSelfLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('profile/page/wishlist/views/self.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originProfilePageWishlistSelf
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} emptywishlistheading message heading for when you haven't added games to your wishlist
     * @param {LocalizedString} emptywishlistdescription message body for when you haven't added games to your wishlist
     * @param {LocalizedString} privateprofileheading mesage heading for when a user's wishilist is hidden due to privacy settings
     * @param {LocalizedString} privateprofiledescription message body for when a user's wishilist is hidden due to privacy settings
     * @param {LocalizedString} sharecta share your wishlist text
     * @param {LocalizedString} shareyourwishlist the inline wishlist sharing text
     * @param {ImageUrl} addtowishlistimage the tutorial animated image for showing users how to add to wishlist
     * @description
     *
     * Profile Page - Wishlist - Self
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-profile-page-wishlist-self
     *            emptywishlistheading="You haven't added any games to your wishlist"
     *            emptywishlistdescription="To add a game... etc."
     *            privateprofileheading="Your profile is set to private"
     *            privateprofiledescription="Your wishlist can't be viewed because of your current privacy settings. Want to share your wishlist with others? Go to EA Account and Billing &gt; Privacy Settings &gt; Who can see my profile  and choose any option except 'No One'."
     *            sharecta="Share your wishlist"
     *            addtowishlistimage="https://assets.example.com/image.jpg"
     *         ></origin-profile-page-wishlist-self>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originProfilePageWishlistSelf', originProfilePageWishlistSelf);
}());

