/**
 * @file cta/purchase.js
 */
(function () {
    'use strict';

    var CLS_ICON_LEARNMORE = 'otkicon-learn-more';

    var CONTEXT_NAMESPACE = 'origin-cta-purchase';

    var GIFT_CTA_CONTEXT_NAMESPACE = 'origin-cta-gift';

    /**
     * BooleanEnumeration
     * @enum {String}
     */
    var BooleanEnumeration = {
        "true": true,
        "false": false
    };

    function OriginCtaPurchaseCtrl($scope, DirectiveScope, StoreActionFlowFactory, UtilFactory, GameRefiner, GiftingFactory, GiftingWizardFactory, WishlistFactory, AuthFactory, QueueFactory, WishlistObserverFactory) {
        var localize = UtilFactory.getLocalizedStrGenerator(CONTEXT_NAMESPACE);
        var unfollowQueueFunc;
        $scope.iconClass = CLS_ICON_LEARNMORE;
        $scope.type = 'primary';
        $scope.disableContextMenu = $scope.disableContextMenu || BooleanEnumeration.false;
        $scope.contextMenuItems = [];

        var modelReadyPromise;

        $scope.strings = {
            gift: UtilFactory.getLocalizedStr(false, GIFT_CTA_CONTEXT_NAMESPACE, 'gift-game-str'),
            wishlistAdd: UtilFactory.getLocalizedStr(false, GIFT_CTA_CONTEXT_NAMESPACE, 'add-wishlist-game-str'),
            wishlistRemove: UtilFactory.getLocalizedStr(false, GIFT_CTA_CONTEXT_NAMESPACE, 'remove-wishlist-game-str'),
            buy: UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'buy-contextmenu')
        };

        function isGiftableAndUnowned() {
            var isOwnedOutright = GameRefiner.isOwnedOutright($scope.model);
            return !isOwnedOutright && GiftingFactory.isGameGiftable($scope.model);
        }

        function getTranslatedContextMenuItems(contextMenuItem) {
            contextMenuItem.label = $scope.strings[contextMenuItem.label];
            return contextMenuItem;
        }

        function getUserWishlist() {
            return WishlistFactory.getWishlist(Origin.user.userPid());
        }

        function addOfferToWishlist(offerId) {
            return function() {
                AuthFactory.promiseLogin().then(function () {
                    getUserWishlist().addOffer(offerId);
                });
            };
        }

        function addOfferToWishlistODC(offerId) {
            if (offerId === $scope.model.offerId) {
                addOfferToWishlist(offerId)();
            }
            if (_.isFunction(unfollowQueueFunc)) {
                unfollowQueueFunc();
            }
        }

        function removeOfferFromWishlist(offerId) {
            return function() {
                getUserWishlist().removeOffer(offerId);
            };
        }

        /**
         * Determine the context menu object for wishlists
         * @return {Function}
         */
        function determineWishlistContextMenu(offerId, offerInWishlist) {
            var wishlistContext = {
                enabled: true,
                divider: false
            };

            if (offerInWishlist) {
                _.merge(wishlistContext,
                        {   label: 'wishlistRemove',
                            callback: removeOfferFromWishlist(offerId),
                            tealiumType: 'Wishlist - Remove'
                        });
            } else {
                _.merge(wishlistContext,
                        {   label: 'wishlistAdd',
                            callback: addOfferToWishlist(offerId),
                            tealiumType: 'Wishlist - Add'
                        });
            }

            return wishlistContext;
        }

        function populateContextMenuItems(inWishlist) {
            if (!$scope.disableContextMenu) {
                var contextMenuItems = [],
                    giftingContextMenu = GiftingWizardFactory.getGiftContextMenu($scope.model);

                if (giftingContextMenu && isGiftableAndUnowned()) {
                    contextMenuItems.push(getTranslatedContextMenuItems(giftingContextMenu));
                    $scope.contextMenuItems = contextMenuItems;
                }

                var wishlistContextMenu = determineWishlistContextMenu(_.get($scope, ['model', 'offerId']), inWishlist);
                if (wishlistContextMenu) {
                    contextMenuItems.push(getTranslatedContextMenuItems(wishlistContextMenu));
                }

                if (contextMenuItems.length > 0) {
                    contextMenuItems.push(getTranslatedContextMenuItems({
                        label: 'buy',
                        callback: $scope.onBtnClick,
                        enabled: true,
                        divider: false,
                        tealiumType: 'Buy Now'
                    }));
                }
                $scope.contextMenuItems = contextMenuItems;
                $scope.$digest();  
            }
        }

        $scope.onBtnClick = function () {
            modelReadyPromise.then(function(game) {
                // Emit an event that is picked up by OriginHomeDiscoveryTileProgrammableCtrl,
                // in order to record cta-click telemetry for the tile.
                $scope.$emit('cta:clicked');

                // Emit an event to remove any info bubble that may be showing
                $scope.$emit('infobubble-remove');

                var action = StoreActionFlowFactory.getPrimaryAction(game);
                StoreActionFlowFactory.startActionFlow(game, action);
            });
        };

        $scope.$watch('model.formattedPrice', function () {
            if (!_.isEmpty($scope.model)) {
                var formattedPrice = $scope.model.formattedPrice,
                    virtualCurrencyMap = {
                        '_BW': localize($scope.biowarepoints, 'biowarepoints', {'%value%': $scope.model.price}),
                        '_FF': localize($scope.fifapoints, 'fifapoints', {'%value%': $scope.model.price}),
                        '_FW': localize($scope.fifaworldpoints, 'fifaworldpoints', {'%value%': $scope.model.price}),
                        '_PF': localize($scope.play4freefunds, 'play4freefunds', {'%value%': $scope.model.price}),
                        '_SC': localize($scope.simscurrency, 'simscurrency', {'%value%': $scope.model.price})
                    };

                if (_.isNumber($scope.model.price) && virtualCurrencyMap.hasOwnProperty($scope.model.currency)) {
                    formattedPrice = virtualCurrencyMap[$scope.model.currency];
                }

                $scope.btnText = UtilFactory.getLocalizedStr(
                    false,
                    CONTEXT_NAMESPACE,
                    StoreActionFlowFactory.getPrimaryAction($scope.model),
                    {'%price%': formattedPrice}
                );

            }
        });

        function onUpdateWishlist(wishlist) {
            var inWishlist = _.some(wishlist.offerList, {id: _.get($scope, ['model', 'offerId'])});
            populateContextMenuItems(inWishlist);
        }

        modelReadyPromise = new Promise(function(resolve) {

            var stopWatchingCollection = $scope.$watchCollection('model', function (game) {
                if (angular.isDefined(_.get(game, 'giftable'))) {
                    if (!$scope.disableContextMenu) {
                        WishlistObserverFactory.getObserver(Origin.user.userPid(), $scope, UtilFactory.unwrapPromiseAndCall(onUpdateWishlist));
                    }

                    stopWatchingCollection();
                    resolve(game);
                }
            });
        });

        $scope.$watchOnce('model.offerId', function(offerId) {
            unfollowQueueFunc = QueueFactory.followQueue('addOfferToWishlistODC', _.partial(addOfferToWishlistODC, offerId));
        });

        //if this button was setup in CQ, we need to take in a path and figure out the model
        function setModel(game) {
            $scope.model = game;
        }

        if ($scope.ocdPath && !$scope.model) {
            DirectiveScope.populateScopeWithOcdAndCatalog($scope, CONTEXT_NAMESPACE, $scope.ocdPath, setModel);
        }

    }


    function originCtaPurchase(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                type: '@',
                model : '=',
                ocdPath: '@',
                disableContextMenu: '@'
            },
            controller: 'OriginCtaPurchaseCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('cta/views/purchase.html')
        };
    }


    /**
     * @ngdoc directive
     * @name origin-components.directives:originCtaPurchase
     * @restrict E
     * @element ANY
     * @scope
     * @param {Object} model The model for the product that is being purchased (selectedModel)
     * @param {string} type type primary
     * @param {LocalizedString} preorder cta pre-order text. Set up once as default.
     * @param {LocalizedString} buy cta buy text. Set up once as default.
     * @param {LocalizedString} buy-addon cta buyAddon text. Set up once as default.
     * @param {BooleanEnumeration} disable-context-menu desc
     * @param {LocalizedString} buy-contextmenu cta context menu (for gifting) text. Set up once in defaults
     * @param {LocalizedString} subscribe cta subscribe text. Set up once as default.
     * @param {LocalizedString} vault cta vault text. Set up once as default.
     * @param {LocalizedString} free cta free text. Set up once as default.
     * @param {LocalizedString} preload cta preload text. Set up once as default
     * @param {LocalizedString} preloaded cta reloaded text. Set up once as default
     * @param {LocalizedString} download cta download text. Set up once as default
     * @param {LocalizedString} play cta play text. Set up once as default
     * @param {LocalizedString} biowarepoints the bioware points string.
     * @param {LocalizedString} fifapoints the fifa points string.
     * @param {LocalizedString} fifaworldpoints the fifa world points string.
     * @param {LocalizedString} play4freefunds the play4free funds string.
     * @param {LocalizedString} simscurrency the sims currency string.
     * @param {LocalizedString} buynow *Update in defaults
     * @param {OcdPath} ocd-path - ocd path of the offer to purchase (ignored if offer id entered)
     *
     * @description
     *
     * Purchase (takes you to shopping cart) call to action button
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-cta-purchase href="http://somepurchaselink" description="Buy me" type="primary"></origin-cta-purchase>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginCtaPurchaseCtrl', OriginCtaPurchaseCtrl)
        .directive('originCtaPurchase', originCtaPurchase);
}());
