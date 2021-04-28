/**
 * @file unveiling/scripts/unveiling.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-pdp-hero',
        CONTEXT_NAMESPACE_UNVEIL = 'origin-unveiling',
        MAX_CENTERED_CHAR_COUNT = 120,
        NEWLINE_REGEX = /(\r\n|\n|\r)/gm;

    function OriginUnveilingCtrl($scope, $stateParams, $window, UtilFactory, ComponentsConfigFactory, DirectiveScope, FeatureDetectionFactory, GamesDataFactory, ObjectHelperFactory, ComponentsLogFactory, AuthFactory, GiftingFactory, AppCommFactory, DialogFactory, ScreenFactory, WishlistFactory, PdpFactory, OfferTransformer) {
        var defaultPackArt = ComponentsConfigFactory.getImagePath('packart-placeholder.jpg'),
            fromSPA = $stateParams.fromSPA;

        // Get offerId from URL and set to scope
        if(decodeURIComponent($stateParams.offerId)) {
            $scope.offerId = decodeURIComponent($stateParams.offerId);
        }

        $scope.locStrings = {};
        $scope.isLoading = true;
        $scope.isMobileView = ScreenFactory.isSmall();
        $scope.isCtaVisible = false;

        // Ms-accelerator is only supported in edge v12 & v13. This disables the blur for those versions
        // to avoid the svg-blur-and-gradient-freakout bug
        $scope.supportsBlur = FeatureDetectionFactory.browserSupports('css-filters') && !FeatureDetectionFactory.browserSupports('ms-accelerator');

        // set the offer id for the OGD page. bundles do not show up in the games library
        // they contain offers that do, so we should link to those, not to the bundle.
        // defaults to the original offer id
        function setOgdOfferId(catalogData) {
            $scope.ogdOfferId = _.get(_.first(_.values(catalogData)), 'ogdOfferId', $scope.offerId);
            $scope.$digest();
        }

        /**
        * getModel
        *
        * Retrieves catalog data and uses DirectiveScope methods
        * to add it to a new local object
        *
        * @param {Object} catalogData
        * @return {Object} returned entitlement data
        */
        function getModel (catalogData) {
            var pdpHeroAttributes = {};

            return DirectiveScope.populateScope(pdpHeroAttributes, CONTEXT_NAMESPACE, catalogData.offerPath).then(function() {
                $scope.packArt = _.get(catalogData, ['i18n', 'packArtLarge'], defaultPackArt );
                $scope.gameTitle = _.get(catalogData, ['i18n', 'displayName'], '');
                $scope.backgroundImage = _.get(pdpHeroAttributes, 'backgroundImage');
                OfferTransformer.transformOffer([catalogData]).then(setOgdOfferId);

                return catalogData;
            });
        }

        /**
        * downloadLaterClick
        *
        * Sends user to game library w/o download
        *
        * @return {void}
        */
        $scope.downloadLaterClick = function () {
            AppCommFactory.events.fire('uiRouter:go', 'app.game_gamelibrary');
        };

        $scope.onAvatarClick = function($event) {
            if ($event) {
                $event.preventDefault();
            }
        };

        function onDialogClose () {
            if(fromSPA) {
                $window.history.back();
            }
            else {
                AppCommFactory.events.fire('uiRouter:go', 'app.home_home');
            }
        }

        /**
        * setLoggedInStrings
        *
        * Sets localized strings to scope
        *
        * Must happen in promise chain to make sure sender name and
        * game title are ready
        *
        */
        function setLoggedInStrings () {
            $scope.giftSenderName = _.get($scope, ['giftData', 'senderName'], '');
            $scope.locStrings.sentByStr = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE_UNVEIL, 'sent-by-str', {'%sender%': $scope.giftSenderName, '%gametitle%': $scope.gameTitle});

            $scope.locStrings.alternateActionLinkStr = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE_UNVEIL, 'alternate-action-link-str');
            $scope.locStrings.newGiftFlashlandingMessageStr = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE_UNVEIL, 'new-gift-flashlanding-message-str');

            $scope.senderMessage = _.get($scope, ['giftData', 'message'], '');
            $scope.locStrings.senderQuoteStr = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE_UNVEIL, 'sender-quote-str', { '%message%': $scope.giftData.message });
        }

        /**
        * handleGiftError
        *
        * Handles errors getting gift data
        * Opens dialog with one option and returns user to homepage
        *
        */
        function handleGiftError () {
            defaultGiftError();
        }

        /**
        * setContentLength
        *
        * This sets a scope property to determine the size (height) of the sender
        * message string.
        *
        * If there are more than 2 newline characters, set as big (tall) object.
        */
        function setContentLength() {
            var message = _.get($scope, ['giftData', 'message']) || '';
            var newlineCount = message.match(NEWLINE_REGEX) || [];

            $scope.isLongContent = (message.length > MAX_CENTERED_CHAR_COUNT) || (newlineCount.length > 2);
        }

        /**
        * processGiftOpen
        *
        * Sets the gift data response object to scope properties needed for view
        *
        * Refreshes view
        *
        */
        function processGiftOpen (response) {
            $scope.giftData = response;

            setLoggedInStrings();
            setContentLength();

            $scope.isLoading = false;

            $scope.$digest();
        }

        // show the downloadinstallplay cta
        function enableGiftingCta() {
            $scope.isCtaVisible = true;
            $scope.$evalAsync();
        }

        // to avoid a race condition on the component loading and the entitlements being updated,
        // bind an event to the entitlements update completion event that shows the CTA only
        // after the entitlements have been updated
        function setEntitlementEvents() {
            GamesDataFactory.events.once('games:baseGameEntitlementsUpdate', enableGiftingCta);
            GamesDataFactory.events.once('games:extraContentEntitlementsUpdate', enableGiftingCta);
        }

        // remove entitlement events
        function onDestroy() {
            GamesDataFactory.events.off('games:baseGameEntitlementsUpdate', enableGiftingCta);
            GamesDataFactory.events.off('games:extraContentEntitlementsUpdate', enableGiftingCta);
        }

        $scope.$on('$destroy', onDestroy);

        /**
         * Removes any lower edition base game offer IDs that are in the wishlist
         * @param {object[]} wishlistOfferList
         * @param {string[]} lowerEditionSiblingOfferIds
         */
        function removeLowerEditionBasegameOfferIdsFromWishlist(wishlist, wishlistOfferList, lowerEditionSiblingOfferIds){
            var wishlistOfferIds = _.pluck(wishlistOfferList, 'id');
            _.forEach(lowerEditionSiblingOfferIds, function(offerId){
                if (wishlistOfferIds.indexOf(offerId) > -1){
                    wishlist.removeOffer(offerId);
                }
            });
        }

        /**
         * Remove any lower edition offer that is in the wishlist
         */
        function updateWishlsit(){
            var wishlist = WishlistFactory.getWishlist(Origin.user.userPid()),
                wishlistOffersPromise = wishlist.getOfferList(),
                lowerEditionSiblingOfferIdPromise = PdpFactory.getLowerEditionBaseGameOfferIdsByOffer($scope.offerId);
            return Promise.all([wishlistOffersPromise, lowerEditionSiblingOfferIdPromise]).then(_.spread(_.partial(removeLowerEditionBasegameOfferIdsFromWishlist, wishlist)));
        }

        /**
        * processGiftData
        *
        * Function chain goes as follows:
        * 1) calls to set gift status to open (Promise<offerId>)
        * 2) calls to get gift info object
        * 3) calls local method to set scope variables for view and digest
        * 4) Refreshes local entitlements with updated game data
        *
        * Error: if any calls fail, calls default gift error which opens
        * a dialog with information and further actions
        *
        * @return {void}
        */
        function processGiftData () {
            GiftingFactory.openGift($scope.offerId)
                .then(processGiftOpen)
                .then(GamesDataFactory.refreshGiftEntitlements)
                .then(updateWishlsit)
                .catch(handleGiftError);
        }


        /**
        * defaultGiftError
        *
        * This handles errors in getting/setting gift info
        * Opens an alert box with one option and sends the user to the homepage
        *
        * @return {void}
        */
        function defaultGiftError () {
            DialogFactory.openAlert({
                id: 'origin-unveiling-gift-procurement-error',
                title: UtilFactory.getLocalizedStr($scope.cantopentitle, CONTEXT_NAMESPACE_UNVEIL, 'cant-open-title-str'),
                description: UtilFactory.getLocalizedStr($scope.cantopendescription, CONTEXT_NAMESPACE_UNVEIL, 'cant-open-description-str'),
                rejectText: UtilFactory.getLocalizedStr($scope.closestr, CONTEXT_NAMESPACE_UNVEIL, 'close-str'),
                callback: onDialogClose
            });
        }

        /**
        * Error handler for catalog information
        *
        * If something borks (ie: offerID from URL isnt valid) redirect 404
        *
        * @param error - error
        * @return {void}
        */
        function handleCatalogError (error) {
            ComponentsLogFactory.error('Unveiling error getting catalog data: ', error);
            defaultGiftError();
        }

        /**
        * Error handler for auth
        *
        * @param error - error
        * @return {void}
        */
        function handleAuthError (error) {
            ComponentsLogFactory.error('Unveiling error getting auth data: ', error);
            $scope.isLoggedIn = false;
        }

        /**
        * processLogin
        *
        * Sets logged in state to scope
        * Sets loading status to false is user is not logged in
        *
        * @param {Object} response
        *
        */
        function processLogin (response) {
            $scope.isLoggedIn = response.isLoggedIn;
            if ($scope.isLoggedIn) {
                setEntitlementEvents();
                showUnveiling();
            } else {
                $scope.locStrings.newGiftFlashlandingLoginStr = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE_UNVEIL, 'new-gift-flashlanding-login-str');
                $scope.isLoading = false;
                $scope.$digest();
            }
        }

        /**
        *  getCatalogInfo
        *
        * Grabs the catalog info and processes it
        *
        * Note: This is called BEFORE getting gift data because if then
        * catalog info fails, then we don't want to modify gift/entitlement info
        *
        * Function chain as follows:
        * 1) Get catalog information for the offerID
        * 2) getModel - sets catalog information to scope (packart, bgImg, etc)
        * 3) Gets gift information
        *
        * Error: Logs to components log and shows user gift error dialog
        *
        *
        */
        function showUnveiling() {
            GamesDataFactory.getCatalogInfo([$scope.offerId])
               .then(ObjectHelperFactory.getProperty($scope.offerId))
               .then(getModel)
               .then(processGiftData)
               .catch(handleCatalogError);
        }

        /**
        * initFromLoginAndPlatformState
        *
        * Bootstraps the unveiling page
        *
        * @return {void}
        *
        */
        function init () {
            AuthFactory.waitForAuthReady()
               .then(processLogin)
               .catch(handleAuthError);
        }

        // Magic happens from here
        init();
    }

    function originUnveiling(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {},
            controller: OriginUnveilingCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('unveiling/views/unveiling.html')
        };
    }
     /**
     * @ngdoc directive
     * @name origin-components.directives:originUnveiling
     * @restrict E
     * @element ANY
     * @description
     * @param {LocalizedString} sent-by-str "%sender% has ent you %gametitle%" use %sender% to place a sender name and %gametitle% to place a game name
     * @param {LocalizedString} alternate-action-link-str "Add to library and Download later"
     * @param {LocalizedString} new-gift-flashlanding-message-str "You've received a new gift!"
     * @param {LocalizedString} new-gift-flashlanding-login-str "Please sign in to see your gift."
     * @param {LocalizedString} sender-quote-str "<<%message%>>" use %message% to place gift message within regional style quotation marks
     * @param {LocalizedString} cant-open-title-str "We can't seem to find that gift"
     * @param {LocalizedString} cant-open-description-str "Sorry about that, please try again later."
     * @param {LocalizedString} close-str "Close"
     *
     * Rendering area for the unveiling of a gift
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-unveiling>
     *         </origin-unveiling>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originUnveiling', originUnveiling);
}());
