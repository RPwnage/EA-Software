(function () {

    'use strict';

    var CONTEXT_NAMESPACE = 'origin-profile-page-wishlist-gametile';

    /**
    * The controller
    */
    function OriginProfilePageWishlistGametileCtrl($scope, ProductInfoHelper, DateHelperFactory, EventsHelperFactory, OfferTransformer, GamesContextFactory, GamesDataFactory, UtilFactory, PdpUrlFactory, moment, WishlistFactory, SubscriptionFactory, GiftFriendListFactory, GiftingFactory) {


        $scope.removeFromWishlistLoc = UtilFactory.getLocalizedStr($scope.removeStr, CONTEXT_NAMESPACE, 'removestr');
        $scope.undoRemoveFromWishlistLoc = UtilFactory.getLocalizedStr($scope.undoStr, CONTEXT_NAMESPACE, 'undostr');
        $scope.orLoc = UtilFactory.getLocalizedStr($scope.orStr, CONTEXT_NAMESPACE, 'orstr');
        $scope.savingsLoc = UtilFactory.getLocalizedStr($scope.saveStr, CONTEXT_NAMESPACE, 'savestr');
        $scope.originAccessDiscountAppliedLoc = UtilFactory.getLocalizedStr($scope.originAccessDiscountAppliedStr, CONTEXT_NAMESPACE, 'originaccessdiscountappliedstr');
        $scope.addedDateLoc = UtilFactory.getLocalizedStr($scope.addedStr, CONTEXT_NAMESPACE, 'addedstr', {
            '%date%': moment($scope.wish.ts).format('LL')
        });
        $scope.playFreeLoc = ['<span class="origin-profile-wishlist-access-text-wrapper origin-telemetry-profile-wishlist-access-text-wrapper">',
            UtilFactory.getLocalizedStr($scope.playFreeStr, CONTEXT_NAMESPACE, 'playfreestr', {
                '%access%': '<div class="wishlist-access origin-access-logo"><i class="otkicon otkicon-originlogo origin-telemetry-otkicon-originlogo"></i><i class="otkicon otkicon-accesstext origin-telemetry-otkicon-accesstext"></i></div>'
            }),
            '</span>'
        ].join('');

        var eligibilityStrings = {
            //keys have to match status coming back from server.
            'not_eligible': UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'not-eligible-str'),
            'ineligible_to_receive': UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'ineligible-to-receive-str'),
            'already_owned': UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'owns-game-str'),
            'unknown': UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'unknown-str')
        };

        var wishlist = WishlistFactory.getWishlist($scope.userId),
            isAvailableFromSubscription = false;

        var isInTheFuture = DateHelperFactory.isInTheFuture;

        $scope.isSelf = ($scope.userId === '') || (Number($scope.userId) === Number(Origin.user.userPid()));
        /**
         * Navigate PDP
         * @method navigateToGame
         */
        $scope.navigateToGame = function() {
            if($scope.model) {
                PdpUrlFactory.goToPdp($scope.model);
            }
        };

        $scope.isFree = function(model) {
            return ProductInfoHelper.isFree(model);
        };

        $scope.showDirectAcquisitionCTA = function() {
            return ($scope.isSelf && !$scope.model.isOwned && (ProductInfoHelper.isFree($scope.model) || (isSubscriptionOffer() && SubscriptionFactory.userHasSubscription())));
        };

        $scope.showPurchaseCTA = function() {
            return ($scope.isSelf && !$scope.model.isOwned && !$scope.model.vaultEntitlement && !ProductInfoHelper.isFree($scope.model));
        };


        $scope.isSubscriber = SubscriptionFactory.userHasSubscription();

        $scope.onRemoveClicked = function($evt) {
            $evt.stopPropagation();
            $evt.preventDefault();

            $scope.removed = true;
            wishlist.removeOffer($scope.wish.id);
        };

        function isSubscriptionOffer() {
            var subscriptionOffer = $scope.model &&
                !$scope.isFree($scope.model) &&
                $scope.model.vaultOcdPath &&
                ($scope.model.subscriptionAvailable || $scope.model.isUpgradeable);

            return (subscriptionOffer || isAvailableFromSubscription) ? true : false;
        }

        $scope.isOwnedSubscriptionOffer = function() {
            return isSubscriptionOffer() && SubscriptionFactory.userHasSubscription();
        };

        $scope.isVaultEntitlement = function() {
            return (isSubscriptionOffer() && SubscriptionFactory.userHasSubscription()) && !$scope.model.oth;
        };

        function setIsAvailableFromSubscription(available) {
            isAvailableFromSubscription = available;
        }

        function setPrimaryAction(action) {
            $scope.primaryAction = action;
        }

        function setModel(data) {
            if (!_.isUndefined(data) && !_.isUndefined(data[$scope.wish.id])) {
                $scope.model = data[$scope.wish.id];
                if (isInTheFuture($scope.model.releaseDate)) {
                    $scope.releaseDateLoc = UtilFactory.getLocalizedStr($scope.releaseDateStr, CONTEXT_NAMESPACE, 'releasedatestr').replace('%date%', moment($scope.model.releaseDate).format('LL'));
                }
            }
        }

        function updateEligibilityStr(eligibility) {

            $scope.$digest();
            if (!$scope.isGameGiftable) {
                if (eligibility) {
                    $scope.eligibilityStr = eligibilityStrings[eligibility];
                } else {
                    $scope.eligibilityStr = _.get(eligibilityStrings, ['not_eligible']);
                }

                if (!$scope.eligibilityStr) {
                    $scope.eligibilityStr =  _.get(eligibilityStrings, ['unknown']);
                }
            }
        }

        function setGiftability() {
            $scope.isGameGiftable = false;
            if (!_.isUndefined($scope.model)) {
                var giftable = GiftingFactory.isGameGiftable($scope.model);
                if (!$scope.isSelf) {
                    if (giftable) {
                        checkUserEligibility($scope.model)
                            .then(updateEligibilityStr)
                            .catch(function() {
                                $scope.eligibilityStr =  _.get(eligibilityStrings, ['unknown']);
                                $scope.$digest();
                            });
                    } else {
                        $scope.isGameGiftable = giftable;
                        updateEligibilityStr();
                    }
                }
            }
        }

        /**
         * Checks users eligibility to receive the game.
         * @param eligibilityArray
         */
        function processEligibility(eligibilityArray) {
            var eligibility = _.get(_.head(eligibilityArray), 'status', 'unknown').toLowerCase();
            if (GiftFriendListFactory.isFriendEligibleToReceive(eligibility)) {
                $scope.isGameGiftable = true;
            }
            return eligibility;
        }

        function checkUserEligibility(model) {
            return GiftFriendListFactory.getFriendsGiftingEligibility(model.offerId, [$scope.userId])
                .then(processEligibility);
        }

        function existsNotDefault(packArtUrl) {
            return (!!packArtUrl && packArtUrl.length > 0 && packArtUrl !== GamesDataFactory.defaultBoxArtUrl());
        }

        function getPackArtUrl() {
            var url = GamesDataFactory.defaultBoxArtUrl();
            if (!_.isUndefined($scope.model)) {
                url = _.filter([$scope.model.i18n.packArtLarge, $scope.model.i18n.packArtMedium, $scope.model.i18n.packArtSmall], existsNotDefault)[0];
            }
            return url;
        }

        function setPackArt() {
            $scope.packArtUrl = getPackArtUrl();
            $scope.usingSmallPackArt = false;

            if (!_.isUndefined($scope.model)) {
                $scope.usingSmallPackArt = ($scope.packArtUrl === $scope.model.i18n.packArtSmall);
            }
        }


        function loadModel() {
            Promise.all([
                GamesContextFactory.primaryAction($scope.wish.id, true)
                    .then(setPrimaryAction),
                GamesDataFactory.isAvailableFromSubscription($scope.wish.id)
                    .then(setIsAvailableFromSubscription),
                GamesDataFactory.getCatalogInfo([$scope.wish.id])
                    .then(OfferTransformer.transformOffer)
                    .then(setModel)
                    .then(setPackArt)
                    .then(setGiftability)
                ]).then(function() {
                    $scope.$digest();
                });
        }
        loadModel();

        function handleGameUpdate() {
            loadModel();
        }
        var eventHandles = [GamesDataFactory.events.on('games:update:' + $scope.wish.id, handleGameUpdate)];

        function onDestroy() {
            EventsHelperFactory.detachAll(eventHandles);
        }

        // when the element is destroyed make sure that we cleanup
        $scope.$on('$destroy', onDestroy);
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originProfilePageWishlistGametile
     * @restrict E
     * @element ANY
     * @scope
     * @param {Object} wish object
     * @param {String} userid - nucleusId
     * @param {LocalizedString} removestr "Remove from Wishlist"
     * @param {LocalizedString} undostr "Undo"
     * @param {LocalizedString} addedstr "Added %date%"
     * @param {LocalizedString} releasedatestr "Release Date: %date%"
     * @param {LocalizedString} orstr "OR"
     * @param {LocalizedString} playfreestr "Play for free with %access%"
     * @param {LocalizedString} savestr "Save %savings%%" use %savings% as a placeholder for the discount number
     * @param {LocalizedString} not-eligible-str Not eligible for gifting text
     * @param {LocalizedString} ineligible-to-receive-str Ineligible to recieve text (already recieved a gift but hasn't opened)
     * @param {LocalizedString} owns-game-str Already owns game text
     * @param {LocalizedString} unknown-str Unable to determine eligibility (gift eligibility service is down)
     * @param {LocalizedString} originaccessdiscountappliedstr Origin Access discount applied
     *
     * @description
     *
     * Profile Page - Wishlist game tile
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-profile-page-wishlist-gametile
     *             wish="{wish object}"
     *             userId="123456677"
     *             removestr="Remove from wishlist"
     *             undostr="undo"
     *             addedstr="Added %date%"
     *             releasedatestr="Release Date: %date%"
     *             orstr="OR"
     *             playfreestr="Play for free with %access%"
     *             savestr="Save %savings%%"
     *             originaccessdiscountappliedstr="Origin Access Discount Applied"
     *         ></origin-profile-page-wishlist-gametile>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginProfilePageWishlistGametileCtrl', OriginProfilePageWishlistGametileCtrl)
        .directive('originProfilePageWishlistGametile', function (ComponentsConfigFactory) {

            return {
                restrict: 'E',
                controller: 'OriginProfilePageWishlistGametileCtrl',
                scope: {
                    wish: '=',
                    userId: '@userid',
                    removeStr: '@removestr',
                    undoStr: '@undostr',
                    addedStr: '@addedstr',
                    releaseDateStr: '@releasedatestr',
                    orStr: '@orstr',
                    playFreeStr: '@playfreestr',
                    saveStr: '@savestr',
                    originAccessDiscountAppliedStr: '@originaccessdiscountappliedstr'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('profile/page/wishlist/views/gametile.html')
            };

        });
}());

