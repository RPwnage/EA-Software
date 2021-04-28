/**
 * @file purchase.js
 */
(function () {
    'use strict';

    // subs checkout constants
    var ERR_SUBS_ALREADY_SUBSCRIBER = 'oax-already-own-subscription',
        ERR_SUBS_MAC_WARNING = 'mac-subscription-purchase',
        SUBS_PREVENTION_REDIRECT = 'app.home_loggedin',
        UNKNOWN_ERROR = 'error-not-known',
        INCOMPATIBLE_PRODUCTS_ERROR = 'incompatible-products',
        OGD_STATE_NAME = 'app.game_gamelibrary.ogd',
        CONTEXT_NAMESPACE = 'origin-dialog-content-prompt',
        SUBS_INTERSTITIAL_ROUTE_OIG = 'app.oig-subscription-interstitial',
        UNVEIL_STATE_NAME = 'app.gift_unveiling',
        ALREADY_USED_TRIAL_ERROR = '7105';

    function PurchaseFactory(AppCommFactory, AuthFactory, CartModel, CheckoutFactory, ComponentsConfigHelper, DialogFactory, ErrorModalFactory, GameRefiner, GamesCatalogFactory, GamesCommerceFactory, GamesDataFactory, GamesEntitlementFactory, UtilFactory, NavigationFactory, ObjectHelperFactory, OcdPathFactory, OfferTransformer, OriginCheckoutTypesConstant, OwnershipFactory, PageThinkingFactory, PurchasePreventionFactory, PurchasePreventionConstant, StoreAgeGateFactory, SubscriptionFactory, VaultRefiner, StoreAccessInterstitialConstants) {

        /**
         * @typedef {object} buyParams
         * @property {string|undefined}  checkoutType type of checkout, to determine if this is for subs
         * @property {boolean|undefined} subsConfirmation if true, user has acknowledged that subs/mac warning
         * @property {string|undefined}  invoiceSource String that identifies source of transaction
         * @property {string|undefined}  promoCode Promo code to be applied
         */
         var isOigContext = ComponentsConfigHelper.isOIGContext();

        /**
         * If the user already owns the game they are trying to buy, we reject the purchase and offer this dialog.
         *
         * @param {Object} game The game data. Gives context to the error message.
         *
         * @private
         */
        function rejectPurchase(game) {
            // Helper function. Ideally this will be refactored into a view that is given to the response dialog.
            function makeOgdLink(game) {
                var ogdHref = NavigationFactory.getHrefFromState(OGD_STATE_NAME, {offerid: game.offerId});
                return '<a href="' + ogdHref + '" class="otka">' + game.i18n.displayName + '</a>';
            }

            // Avoid showing a link to OGD if we're in OIG.  We should not allow the user to access Game Library in OIG per production.
            var title = isOigContext ? game.i18n.displayName : makeOgdLink(game);
            ErrorModalFactory.showDialog('youownthis', {'%edition%': title});
        }

        /**
         * If user is already a subscriber, redirect to home and show error
         *
         * @param  {string} checkoutType type of checkout, to determine if this is for subs
         * @return {Promise} returns a Promise.reject(...) is user is already a subscriber
         */
        function breakPromiseChainIfSubscriber(checkoutType) {
            if ((checkoutType === OriginCheckoutTypesConstant.SUBS || checkoutType === OriginCheckoutTypesConstant.CHAINED) && SubscriptionFactory.userHasSubscription()) {
                AppCommFactory.events.fire('uiRouter:go', SUBS_PREVENTION_REDIRECT);
                return Promise.reject(ERR_SUBS_ALREADY_SUBSCRIBER);
            }
        }

        /**
         * Halt checkout with error (which will trigger Mac Warning modal) if
         * user is on Mac and checkout is for subs
         *
         * @param {string} checkoutType type of checkout, to determine if this is for subs
         * @param {boolean} subsConfirmation
         */
        function breakPromiseChainIfMacUser(checkoutType, subsConfirmation) {
            if (checkoutType === OriginCheckoutTypesConstant.SUBS && !subsConfirmation && Origin.utils.os() === Origin.utils.OS_MAC) {
                return Promise.reject(ERR_SUBS_MAC_WARNING);
            }
        }

        /**
         * Handle checkout errors
         *
         * @param {string} offerId
         * @param {string|object} error data attached to error
         */
        function handleCheckoutError(offerId, error) {
            //stop the performance timer
            Origin.performance.endTime('COMPONENTSPERF:checkoutCart');
            if (error === ERR_SUBS_MAC_WARNING) {
                DialogFactory.open({
                    id: 'subs-mac-warning-modal',
                    xmlDirective: '<origin-dialog-subsmacwarning class="origin-dialog-content" dialog-id="subs-mac-warning-modal" offer-id="' + offerId + '"></origin-dialog-subsmacwarning>',
                    size: 'medium'
                });
            }
            else {
                ErrorModalFactory.showDialog(error);
            }
        }

        function handlePurchasePreventionChoice(result, game, buyParams){
            if (result.accepted){
                purchaseOffer(game, buyParams);
            }
        }

        function addPromoCode(promoCode) {
            if (promoCode) {
                return CartModel.addCoupon(promoCode);
            }
            return Promise.resolve();
        }

        function purchaseOffer(game, buyParams){
            Origin.performance.beginTime('COMPONENTSPERF:checkoutCart');
            CartModel.addOffer(game.offerId)
                .then(_.partial(addPromoCode, buyParams.promoCode))
                .then(_.partial(breakPromiseChainIfSubscriber, buyParams.checkoutType))
                .then(_.partial(breakPromiseChainIfMacUser, buyParams.checkoutType, buyParams.subsConfirmation))
                .then(_.partial(CheckoutFactory.checkout, buyParams.checkoutType, {invoiceSource: buyParams.invoiceSource}))
                .catch(_.partial(handleCheckoutError, game.offerId));
        }

        function giftOffer(game, giftRecipientNucleusId, senderName, senderMessage, giftParams) {
            Origin.performance.beginTime('COMPONENTSPERF:checkoutCart');

            var checkoutParams = {invoiceSource: giftParams.invoiceSource};

            CartModel.addGiftOffer(game.offerId, giftRecipientNucleusId, senderName, senderMessage)
                .then(_.partial(addPromoCode, giftParams.promoCode))
                .then(_.partial(CheckoutFactory.checkout, giftParams.checkoutType, checkoutParams))
                .catch(_.partial(handleCheckoutError, game.offerId))
                .catch(ErrorModalFactory.showDialog);
        }

        /**
         * When base game is not purchasable, give the user a choice to buy it anyway
         */
        function handleNoBaseGamePurchasePreventionChoice(result, offerId, buyParams) {
            if (result.accepted){
                GamesCatalogFactory
                    .getCatalogInfo([offerId])
                    .then(OfferTransformer.transformOffer)
                    .then(_.partialRight(initiateAddonPurchase, buyParams));
            }
        }

        function handleBasegameRequiredError() {
            DialogFactory.openAlert({
                 id: 'web-basegame-notfound-warning',
                 title: UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'basegamenotfoundtitle'),
                 description: UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'basegamenotfounddesc'),
                 rejectText: UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'closeprompt')
             });
        }

        function openBaseGameMissingDialog(game) {
            DialogFactory.open({
                id: PurchasePreventionConstant.DLC_DIALOG_ID,
                xmlDirective: PurchasePreventionFactory.getDLCPurchasePreventionDirective(game, null, false, false),
                size: 'large'
            });

            AppCommFactory.events.once('dialog:hide', _.partialRight(handleNoBaseGamePurchasePreventionChoice, game.offerId));
        }

        /**
         * Opens base game required dialog given a game without base game
         * @param {object} game
         *
         * @private
         */
        function openBaseGameRequiredDialog(game, buyParams){
            GamesDataFactory.getLowestRankedPurchsableBasegame(game).then(function (offerId) {
                if (offerId) {
                    GamesDataFactory.getCatalogInfo([offerId]).then(function (baseGame) {
                        DialogFactory.open({
                            id: PurchasePreventionConstant.DLC_DIALOG_ID,
                            xmlDirective: PurchasePreventionFactory.getDLCPurchasePreventionDirective(game, baseGame[offerId], false, GameRefiner.isCurrency(game)),
                            size: 'large'
                        });
                        AppCommFactory.events.once('dialog:hide', _.partialRight(handleNoBaseGamePurchasePreventionChoice, game.offerId, buyParams));
                    }).catch(handleBasegameRequiredError);
                } else { //master title or base game doesn't exist
                    openBaseGameMissingDialog(game);
                }
            });
        }

        /**
         * Handle response from checkout purchase prevention.
         *
         * @param {object}  game Game data that is returned from Games Data Factory. Has ownership info.
         * @param {buyParams} buyParams Optional parameters for buying
         *
         */
        function handlePurchasePreventionResult(game, buyParams) {

            return function (result) {
                if (result.success) {
                    Origin.performance.endTimeMeasureAndSendTelemetry('COMPONENTSPERF:checkBuy');
                    purchaseOffer(game, buyParams);
                } else {
                    Origin.performance.endTime('COMPONENTSPERF:checkBuy');
                    if (result.reason === PurchasePreventionConstant.OWNED_OFFER) {
                        rejectPurchase(game);
                    } else if (result.reason === PurchasePreventionConstant.BASE_GAME_REQUIRED) {
                        openBaseGameRequiredDialog(game, buyParams);
                    } else if (result.reason === PurchasePreventionConstant.INCORRECT_INPUT) {
                        ErrorModalFactory.showDialog('bad-input');
                    } else if ( result.reason === PurchasePreventionConstant.OWNED_SOME_GAME_IN_OFFER ){
                        DialogFactory.open({
                            id: PurchasePreventionConstant.BUNDLE_DIALOG_ID,
                            xmlDirective: PurchasePreventionFactory.getBundlePurchasePreventionDirectiveXML(result.bundle, true),
                            size: 'large'
                        });
                        AppCommFactory.events.once('dialog:hide', _.partialRight(handlePurchasePreventionChoice, game, buyParams));
                    } else {
                        ErrorModalFactory.showDialog(UNKNOWN_ERROR);
                    }
                }
            };
        }

        /**
         * Handler for addon purchase prevention result
         * Halt on first error
         * @param results
         * @param {Array} offerIds List of offerIds to buy
         */
        function handleAddonPurchasePreventionResult(results) {
            var hasError = false;

            _.forEach(results, function (result) {
                if (!result.success) {
                    if (result.reason === PurchasePreventionConstant.BASE_GAME_REQUIRED) {
                        openBaseGameRequiredDialog(result.product);
                    } else if (result.reason === PurchasePreventionConstant.OWNED_OFFER) {
                        rejectPurchase(result.product);
                    } else {
                        ErrorModalFactory.showDialog(UNKNOWN_ERROR);
                    }
                    hasError = true;
                }
            });

            if (!hasError){
                var products = _.map(results, function (result) {
                    return result.product;
                });
                Origin.performance.endTimeMeasureAndSendTelemetry('COMPONENTSPERF:checkBuyAddOns');
                initiateAddonPurchase(products)
                    .catch(ErrorModalFactory.showDialog);
            } else {
                Origin.performance.endTime('COMPONENTSPERF:checkBuyAddOns');
            }

        }

        /**
         * Handles unopened gift exceptions
         *
         * @param result
         */
        function handleGiftingCheckException(result){
            Origin.performance.endTime('COMPONENTSPERF:checkBuy');
            if (result && result.reason === PurchasePreventionConstant.UNOPENED_GIFT) {
                AppCommFactory.events.fire('uiRouter:go', UNVEIL_STATE_NAME, {offerId : result.offerId, fromSPA: true});
            }
        }
        /**
         * Rejects purchase if already owned. Otherwise continues with purchase flow.
         * @param {object}  game Game data that is returned from Games Data Factory. Has ownership info.
         * @param {buyParams} buyParams Optional params for purchasing
         *
         * @private
         */
        function initiatePurchase(game, buyParams) {
            PageThinkingFactory.unblockUI();
            PurchasePreventionFactory.checkUnopenedGift(game)
                .then(_.partial(PurchasePreventionFactory.checkProduct, game))
                .then(handlePurchasePreventionResult(game, buyParams))
                .catch(handleGiftingCheckException);
        }

        /**
         * stops the performance timer
         *
         * @private
         */
        function endPerformanceTimerBuy(result) {
            Origin.performance.endTime('COMPONENTSPERF:checkBuy');
            return Promise.reject(result);
        }

        /**
         * Gets entitlements for the game and initiates the purchase.
         *
         * @param {string}  offerId The game data of the game trying to be bought.
         * @param {buyParams}  buyParams Optional params for purchasing
         */
        function buy(offerId, buyParams) {
            buyParams = buyParams || {};
            buyParams.checkoutType = buyParams.checkoutType || OriginCheckoutTypesConstant.DEFAULT;

            Origin.performance.beginTime('COMPONENTSPERF:checkBuy');

            PageThinkingFactory.blockUI();
            OwnershipFactory.getGameDataWithEntitlements(offerId)
                .then(_.partial(validateUserAge, offerId))
                .then(PageThinkingFactory.unblockUIPromise)
                .then(_.partialRight(initiatePurchase, buyParams))
                .catch(PageThinkingFactory.unblockUIPromiseOnError)
                .catch(endPerformanceTimerBuy)
                .catch(ErrorModalFactory.showDialog);
        }

        /**
         * Initiates subscription purchase flow. launches interstitial window if more than
         * one subscription offer id is merchandised
         *
         * @param {string}  offerIds The offer id(s) being purchased
         */
        function subscriptionCheckout(offerIds) {
            if(offerIds) {
                var offers = _.map(offerIds.split(','), function(value){ return value.trim(); });
                var buyParams = {
                    checkoutType: OriginCheckoutTypesConstant.SUBS,
                    subsConfirmation: false
                };

                if(SubscriptionFactory.userHasSubscription()) {
                    return handleCheckoutError(offers, ERR_SUBS_ALREADY_SUBSCRIBER);
                }

                if(_.size(offers) > 1) {
                    if (ComponentsConfigHelper.isOIGContext()){
                        AppCommFactory.events.fire('uiRouter:go', SUBS_INTERSTITIAL_ROUTE_OIG);
                    } else {
                        DialogFactory.openDirective({
                            id: StoreAccessInterstitialConstants.id,
                            name: 'origin-dialog-content-templateloader',
                            size: 'large',
                            data: {
                                template: StoreAccessInterstitialConstants.template,
                                modal: StoreAccessInterstitialConstants.id
                            }
                        });
                    }
                } else {
                    buy(offerIds, buyParams);
                }
            }
        }

        /**
         * checks catalog data for subs trial offers
         *
         * @param {array}  catalogData The catalog data for the offers being purchased
         * @return {Promise} resolves if no trial offers, or user has not cosumed the trial already
         */
        function checkForFreeTrial(catalogData) {
            var containsFreeTrial = false;

            _.forEach(catalogData, function(offer) {
                if(VaultRefiner.isSubscriptionFreeTrial(offer)) {
                    containsFreeTrial = true;

                    return;
                }
            });

            if(containsFreeTrial && SubscriptionFactory.hasUsedFreeTrial()) {
                return Promise.reject(ALREADY_USED_TRIAL_ERROR);
            } else {
                return Promise.resolve();
            }
        }

        /**
         * checks a subscription purchase for trial offer status
         *
         * @param {string}  offerIds The offer ids being purchased
         * @return {Promise} resolves if no trial offers, or user has not cosumed the trial already
         */
        function verifyTrialStatus(offerIds) {
            var offers = _.map(offerIds.split(','), function(value){ return value.trim(); });

            return SubscriptionFactory.retrieveUserTrialInfo()
                    .then(_.partial(GamesDataFactory.getCatalogInfo, offers))
                    .then(checkForFreeTrial);
        }

        /**
         * Gets entitlements for the game and initiates the purchase.
         *
         * @param {string}  giftData The game data of the game trying to be bought.
         * @param {buyParams}  giftParams Optional params for purchasing
         */
        function gift(giftData, giftParams) {
            giftParams = giftParams || {};
            var offerId = _.get(giftData, 'offerId');
            var recipientId = _.get(giftData, 'recipientId');
            var message = _.get(giftData, 'message');
            var senderName = _.get(giftData, 'senderName');
            giftParams.checkoutType = giftParams.checkoutType || OriginCheckoutTypesConstant.GIFT;

            Origin.performance.beginTime('COMPONENTSPERF:checkBuy');

            PageThinkingFactory.blockUI();
            GamesDataFactory.getCatalogInfo([offerId])
                .then(_.partial(validateUserAge, offerId))
                .then(_.partialRight(giftOffer, recipientId, senderName, message, giftParams))
                .then(PageThinkingFactory.unblockUIPromise)
                .catch(PageThinkingFactory.unblockUIPromiseOnError)
                .catch(endPerformanceTimerBuy)
                .catch(ErrorModalFactory.showDialog);
        }

        /**
         * stops the performance timer
         *
         * @private
         */
        function endPerformanceTimerBuyAddOn(result) {
            Origin.performance.endTime('COMPONENTSPERF:checkBuyAddOns');
            return Promise.reject(result);
        }

        /**
         * Gets entitlements for the addons and initiates the purchase.
         *
         * @param {[string]} offerIds The offer IDs of the DLC trying to be bought.
         */
        function buyAddons(offerIds) {
            Origin.performance.beginTime('COMPONENTSPERF:checkBuyAddOns');
            // Ensure the passed value is an array
            offerIds = ObjectHelperFactory.wrapInArray(offerIds);
            PageThinkingFactory.blockUI();
            GamesDataFactory.getCatalogInfo(offerIds)
                .then(_.partial(validateCatalogInfo, offerIds))
                .then(OfferTransformer.transformOffer)
                .then(_.partial(validateUserAge, offerIds))
                .then(PageThinkingFactory.unblockUIPromise)
                .then(PurchasePreventionFactory.checkProducts)
                .then(handleAddonPurchasePreventionResult)
                .catch(PageThinkingFactory.unblockUIPromiseOnError)
                .catch(endPerformanceTimerBuyAddOn)
                .catch(ErrorModalFactory.showDialog);
        }

        /**
         * Validates the given catalog info to ensure that all offer data is present
         * @param {[string]} offerIds The list of offerIds that should be present in the catalog info object.
         * @param {object} catalogInfo The data to validate
         * @return {Promise} Resolves into catalog data if the catalog data is present.  Rejects otherwise.
         */
        function validateCatalogInfo(offerIds, catalogInfo) {
            var valid = true;
            _.forEach(offerIds, function(offerId) {
                if (!catalogInfo.hasOwnProperty(offerId)) {
                    // No catalog info means we can't look up ODC profile data.
                    valid = false;
                    return false;
                }
            });
            return valid ? Promise.resolve(catalogInfo) : Promise.reject('invalid-products');
        }

        function endPerformaceTimeOnError(error) {
            Origin.performance.endTime('COMPONENTSPERF:checkoutCart');
            return Promise.reject(error);
        }

        /**
         * Validates the checkout type, builds a cart, and starts checkout flow.
         * @param {[object]} games Game data. Has ownership, catalog, price, and ODC profile info.
         *
         * @private
         */
        function initiateAddonPurchase(games, buyParams) {
            var addonOfferIds = [],
                subscriptionPlanIds = [],
                fictionalCurrencyCode = '',
                category = '',
                invoiceSource = '',
                checkoutType = '';

            _.forEach(games, function(game) {
                if (game.fictionalCurrencyCode) {
                    fictionalCurrencyCode = game.fictionalCurrencyCode;
                }
                if (game.categoryId) {
                    category = game.categoryId;
                }
                if (game.invoiceSource) {
                    invoiceSource = game.invoiceSource;
                }

                if (GameRefiner.isSubscription(game)) {
                    subscriptionPlanIds.push(game.offerId);
                } else {
                    addonOfferIds.push(game.offerId);
                }
            });

            if (games.length > 1 && fictionalCurrencyCode) {
                // Cannot purchase more than 1 offer priced in virtual currency at a time.
                // This also prevents mixing offers priced in virtual currency and real currency, and
                // offers priced in two different virtual currencies (i.e. BioWare and FIFA Points).
                // None of these scenarios can be supported by a single cart.
                return Promise.reject(INCOMPATIBLE_PRODUCTS_ERROR);
            }

            if (subscriptionPlanIds.length > 1 || addonOfferIds.length === 0) {
                // Cannot purchase more than 1 subscription offer.  The user must also purchase at least one
                // non-subscription offer with the subscription offer to use chained checkout.
                return Promise.reject(INCOMPATIBLE_PRODUCTS_ERROR);
            }

            if (subscriptionPlanIds.length > 0) {
                checkoutType = OriginCheckoutTypesConstant.CHAINED;
            } else if (isOigContext) {
                checkoutType = fictionalCurrencyCode ? OriginCheckoutTypesConstant.TOPUP_OIG : OriginCheckoutTypesConstant.DEFAULT_OIG;
            } else {
                checkoutType = fictionalCurrencyCode ? OriginCheckoutTypesConstant.TOPUP : OriginCheckoutTypesConstant.DEFAULT;
            }

            Origin.performance.beginTime('COMPONENTSPERF:checkoutCart');

            return addAddonOffersToCart(checkoutType, addonOfferIds, fictionalCurrencyCode, buyParams)
                .then(_.partial(breakPromiseChainIfSubscriber, checkoutType))
                .then(_.partial(checkExistingBalance, games, fictionalCurrencyCode))
                .then(_.partialRight(initiateAddonCheckout, checkoutType, fictionalCurrencyCode, category, invoiceSource, _.head(subscriptionPlanIds)))
                .catch(endPerformaceTimeOnError)
                .catch(ErrorModalFactory.showDialog);
        }

        /**
         * Adds addon/subscription offers to the appropriate cart(s) based on checkout type.
         *
         * @param {string} checkoutType Type of checkout, to determine if this is chained checkout
         * @param {[string]} addonOfferIds List of offer IDs being purchased
         * @param {[string]} subscriptionOfferIds List of subscription offer IDs to purchase.
         * @param {string} fictionalCurrencyCode The three letter virtual currency code, i.e. '_BW'
         *
         */
        function addAddonOffersToCart(checkoutType, addonOfferIds, fictionalCurrencyCode, buyParams) {
            var cartParams = {
                currency: fictionalCurrencyCode
            };
            var promoCode = buyParams ? buyParams.promoCode : '';
            return CartModel
                .addOfferList(addonOfferIds, cartParams)
                .then(_.partial(addPromoCode, promoCode));
        }

        /**
         * If the game uses virtual currency, check the user's virtual currency balance.
         *
         * @param {[object]} games Game data. Has ownership, catalog, price, and ODC profile info.
         * @param {string} fictionalCurrencyCode The three letter currency code, i.e. '_BW'
         */
        function checkExistingBalance(games, fictionalCurrencyCode) {
            _.forEach(games, function(game) {
                game.hasSufficientBalance = false;
            });

            if (games.length === 1 && fictionalCurrencyCode) {
                var game = games[0];
                return GamesCommerceFactory.getWalletBalance(fictionalCurrencyCode)
                    .then(_.partial(determineSufficientBalance, game))
                    .then(function() {
                        return games;
                    });
            } else {
                return games;
            }
        }

        /**
         * Determines whether the user has sufficient balance to purchase the offer and updates the model accordingly.
         *
         * @param {object} game Game data. Has ownership, catalog, price, and ODC profile info.
         * @param {Number} balance The user's virtual currency balance.
         */
        function determineSufficientBalance(game, balance) {
            if (_.isObject(game)) {
                game.hasSufficientBalance = (balance >= game.price);
            }
        }

        /**
         * Check out using the appropriate checkout flow for the DLC offers
         *
         * @param {[object]} games Game data. Has ownership, catalog, price, and ODC profile info.
         * @param {string} checkoutType type of checkout, to determine if this is for subs
         * @param {string} fictionalCurrencyCode The three letter virtual currency code, i.e. '_BW'
         * @param {string} category The category ID to use during Lockbox topup flow checkout
         * @param {string} invoiceSource String that identifies source of transaction (the name of the game integrator)
         * @param {string} subscriptionPlanId The subscription service's offer id used for chained checkout
         *
         */
        function initiateAddonCheckout(games, checkoutType, fictionalCurrencyCode, category, invoiceSource, subscriptionPlanId) {
            if (games.length === 1 && games[0].hasSufficientBalance) {
                return CheckoutFactory.checkoutUsingBalance(games[0].offerId);
            } else {
                return CheckoutFactory.checkout(checkoutType, {
                    virtualCurrency: fictionalCurrencyCode,
                    category: category,
                    invoiceSource: invoiceSource,
                    offerId: subscriptionPlanId
                });
            }
        }

        /**
         * Gets entitlements for the game and initiates the purchase.
         * @param {string} offerId The game data of the game trying to be bought.
         * @return {void}
         */
        function entitlementSuccess(offerId) {
            if (offerId) {
                CheckoutFactory.openCheckoutConfirmation([offerId]);
            }
        }

        /**
         * Handler for direct entitlement response
         *
         * @param {string} offerId
         * @param response Promise response
         */
        function vaultEntitlementHandler(offerId, response) {
            var success = ObjectHelperFactory.getProperty(['fulfillmentResponse', 'subscriptionUris'])(response) || false;

            if (success !== false) {
                entitlementSuccess(offerId);
                return Promise.resolve(response);
            } else {
                var cause = ObjectHelperFactory.getProperty(['extra', 'cause'])(response) || false;

                if (cause === 'upgrade-prevention') {
                    // trigger upgrade prevention dialog
                    DialogFactory.open({
                        id: 'vault-upgrade-savegame-warning',
                        xmlDirective: '<origin-dialog-vaultupgradewarning class="origin-dialog-content" dialog-id="vault-upgrade-savegame-warning" offer-id="' + offerId + '"></origin-dialog-vaultupgradewarning>',
                        size: 'large'
                    });
                }
                else if (cause === 'dlc-upgrade-prevention') {
                    // trigger upgrade prevention dialog
                    DialogFactory.open({
                        id: 'vault-dlc-upgrade-warning',
                        xmlDirective: '<origin-dialog-vaultdlcupgradewarning class="origin-dialog-content" dialog-id="vault-dlc-upgrade-warning" offer-id="' + offerId + '"></origin-dialog-vaultdlcupgradewarning>',
                        size: 'medium'
                    });
                }
                else if (cause === 'user-under-age') {
                    ErrorModalFactory.showDialog('user-under-age');
                }
                else {
                    ErrorModalFactory.showDialog('vault-entitlement-failed');
                }


                return Promise.reject(response);
            }
        }

        /**
         * Handle vault entitlement error response
         *
         * @param  {object} data vault entitlement error response
         * @return {object}      parsable object for use in vaultEntitlementHandler
         */
        function vaultEntitlementErrorHandler(data) {
            return {success: false, extra: {cause: data}};
        }

        /**
         * Extract editions from OCD response
         *
         * @param  {object} response response from GamesDataFactory.getOcdByOfferId
         * @return {string[]}        array of offer paths
         */
        function getEditionPathsFromOcdResponse(response) {
            var ocdPath = ObjectHelperFactory.getProperty(['gamehub', 'keys', 'sling'])(response) || undefined;
            var data = _.values(ObjectHelperFactory.getProperty(['gamehub', 'siblings'])(response) || {});

            // push vault offer into data collection
            if (ocdPath) {
                data.push(ocdPath);
            }

            return data;
        }

        /**
         * Reject entitlement attempt if user owns lesser edition or
         * is attempting to entitle DLC through the vault
         *
         * @param {string} vaultOfferId vault offer attempting to be granted
         * @param {object} offers     response from OcdPathFactory.get
         */
        function vaultEntitlementRejectIfLesserOwned(vaultOfferId, offers) {
            var vaultOfferWeight = 0,
                showSubsSaveGameWarning,
                weights = {},
                dlcUpgradeWarning = true;

            _.forEach(offers, function (offer) {
                if (offer.hasOwnProperty('offerId') && offer.offerId === vaultOfferId) {
                    vaultOfferWeight = offer.weight || 0;
                    showSubsSaveGameWarning = _.get(offer, ['platforms', Origin.utils.os(), 'showSubsSaveGameWarning'], false);
                }

                if(_.get(offer, 'offerId') !== vaultOfferId && _.get(offer, 'isOwned', false)) {
                    weights[offer.offerId] = offer.weight || 0;
                }

                // dlc warning logic
                if(
                    offer.hasOwnProperty('offerId') &&
                    offer.offerId === vaultOfferId &&
                    dlcUpgradeWarning
                ) {
                    // not a dlc, dont spwan warning
                    dlcUpgradeWarning = false;
                }
            });

            // reject if lesser weighted owned sibling exists
            if (dlcUpgradeWarning) {
                return Promise.reject('dlc-upgrade-prevention');
            }

            // reject if lesser weighted owned sibling exists
            if (showSubsSaveGameWarning) {
                var rejected = false;

                _.forEach(weights, function (weight) {
                    if (weight < vaultOfferWeight && !rejected) {
                        rejected = true;
                    }
                });

                if(rejected) {
                    return Promise.reject('upgrade-prevention');
                }
            }
        }

        /**
         * Determine whether vault entitlement save-game warning needs to be displayed
         *
         * @param  {string} offerId              Vault offer being entitled
         * @param  {boolean} warningAcknowledged If true, disables warning modal and goes straight to entitlement
         */
        function vaultEntitlementUpgradePrevention(offerId, warningAcknowledged) {
            // if user has acknowleged warning, do not do upgrade prevention check
            if (!warningAcknowledged) {
                return GamesDataFactory.getOcdByOfferId(offerId)
                    .then(getEditionPathsFromOcdResponse)
                    .then(OcdPathFactory.promiseGet)
                    .then(_.partial(vaultEntitlementRejectIfLesserOwned, offerId));
            }
        }

        /**
         * Checks if supplied offer is the vault edition and returns offer id
         * of the vault edition if different
         *
         * @param {object} data
         * @return {string} offerId - the correct offer id to entitle
         */
        function selectVaultEdition(data) {
            var model = _.first(_.values(data));

            if(model.vaultOcdPath && model.offerPath && model.vaultOcdPath !== model.offerPath) {
                return GamesDataFactory.getOfferIdByPath(model.vaultOcdPath, !VaultRefiner.isShellBundle(model));
            } else {
                return model.offerId;
            }
        }

        /**
         * gets the model for the supplied offer id
         *
         * @param {string} offerId The game offer id trying to be entitled.
         * @return {Promise}
         */
        function checkVaultEdition(offerId) {
            return GamesDataFactory
                .getCatalogInfo([offerId])
                .then(selectVaultEdition);
        }

        /**
         * Gets entitlements for the vault game and initiates the purchase.
         *
         * @param {string} offerId The game data of the vault game trying to be entitled.
         * @param {boolean} warningAcknowledged
         * @return {Promise}
         */
        function entitleVaultGame(offerId, warningAcknowledged) {
            warningAcknowledged = warningAcknowledged || false;

            return AuthFactory.promiseLogin()
                .then(_.partial(getCatalogData, offerId))
                .then(_.partial(validateUserAge, offerId))
                .then(PageThinkingFactory.blockUIPromise)
                .then(_.partial(vaultEntitlementUpgradePrevention, offerId, warningAcknowledged))
                .then(_.partial(checkVaultEdition, offerId))
                .then(GamesDataFactory.vaultEntitle)
                .catch(vaultEntitlementErrorHandler)
                .then(_.partial(vaultEntitlementHandler, offerId))
                .then(PageThinkingFactory.unblockUIPromise)
                .catch(PageThinkingFactory.unblockUI);
        }

        /**
         * Handler for direct entitlement response
         *
         * @param offerId
         * @param response Promise response
         */
        function directEntitlementHandler(offerId, response) {
            if (response && response.success) {
                entitlementSuccess(offerId);
                return Promise.resolve(response);
            } else if (response && !response.success) {
                var errorMessage = ObjectHelperFactory.defaultTo('')(ObjectHelperFactory.getProperty(['extra', 'cause'])(response));
                ErrorModalFactory.showDialog(errorMessage);
                return Promise.reject(response);
            }
        }

        /**
         * Check user age against game rating
         *
         * @param offerIds
         * @param game
         */
        function validateUserAge(offerIds, game) {
            if (_.isArray(offerIds)) {
                var promises = [];
                _.forEach(offerIds, function(offerId) {
                    promises.push(StoreAgeGateFactory.validateUserAgePromise(game[offerId]));
                });
                return Promise.all(promises);
            } else {
                return StoreAgeGateFactory.validateUserAgePromise(game[offerIds]);
            }
        }

        /**
         *
         * @param offerId
         * @returns {*|Object|Promise.<Object.<string, catalogInfo>>}
         */
        function getCatalogData(offerId) {
            return GamesCatalogFactory.getCatalogInfo([offerId]);
        }

        /**
         * Initiate the flow of free game direct entitlement
         * @param offerId
         * @returns {Promise} resolves http response
         */
        function directEntitleFreeGame(offerId){
            return GamesDataFactory.directEntitle(offerId)
                .then(_.partial(directEntitlementHandler, offerId))
                .then(PageThinkingFactory.unblockUIPromise);
        }

        /**
         * Handles results from purchase prevention check product function
         * @param offerId
         * @param result
         * @returns {Promise}
         */
        function handleFreeGamePurchasePreventionResult(offerId, result) {
            if (!result.success) {
                return Promise.reject(result);
            } else {
                return directEntitleFreeGame(offerId);
            }
        }

        /**
         * Handles user choice from free DLC purchase prevention modal
         *
         * @param {object} result modal response
         * @param {string} offerId
         *
         * @private
         */
        function handleFreeGamePurchasePreventionChoice(result, offerId){
            if (result.accepted){
                PageThinkingFactory.blockUI();
                directEntitleFreeGame(offerId)
                    .catch(PageThinkingFactory.unblockUIPromiseOnError)
                    .catch(handleFreeGameEntitlementError);
            }
        }

        function openFreeDLCBaseGameRequiredDialog(game, baseGame){
            DialogFactory.open({
                id: PurchasePreventionConstant.DLC_DIALOG_ID,
                xmlDirective: PurchasePreventionFactory.getDLCPurchasePreventionDirective(game, baseGame, true),
                size: 'large'
            });
            AppCommFactory.events.once('dialog:hide', _.partialRight(handleFreeGamePurchasePreventionChoice, game.offerId));
        }

        function getBaseGameCatalogAndOpenDialog(baseGameOfferId, game){
            var offerIds = ObjectHelperFactory.wrapInArray(baseGameOfferId);
            GamesDataFactory.getCatalogInfo(offerIds)
                .then(function (baseGame) {
                    openFreeDLCBaseGameRequiredDialog(game, baseGame[baseGameOfferId]);
                });
        }

        /**
         * Handles  free games entitlement error
         *
         * @param {object} result
         */
        function handleFreeGameEntitlementError(result) {
            if (_.isString(result)){
                result = {reason: result};
            }
            if (result.reason === PurchasePreventionConstant.BASE_GAME_REQUIRED) {
                GamesDataFactory.getLowestRankedPurchsableBasegame(result.product)
                    .then(_.partialRight(getBaseGameCatalogAndOpenDialog, result.product));
            } else {
                ErrorModalFactory.showDialog(result.reason);
            }
        }

        /**
         * Gets entitlements for the game and initiates the purchase.
         * @param {string} offerId The game data of the game trying to be bought.
         * @return {Promise}
         */
        function entitleFreeGame(offerId) {
            return AuthFactory.promiseLogin()
                .then(PageThinkingFactory.blockUIPromise)
                .then(_.partial(getCatalogData, offerId))
                .then(OfferTransformer.loadPrices) // Need to know if price === 0
                .then(_.partial(validateUserAge, offerId))
                .then(PurchasePreventionFactory.checkProduct)
                .then(_.partial(handleFreeGamePurchasePreventionResult, offerId))
                .catch(PageThinkingFactory.unblockUIPromiseOnError)
                .catch(handleFreeGameEntitlementError);
        }

        function renewSubscription() {
            var subscriptionOfferId = SubscriptionFactory.getUserLastSubscriptionOfferId(),
                buyParams = {
                    checkoutType: OriginCheckoutTypesConstant.SUBS
                };
            buy(subscriptionOfferId, buyParams);
        }

        /**
         * In the case where the lesser base game is known, locate and entitle thevault edition
         * @param  {String} offerId the offerId to search
         */
        function entitleVaultGameFromLesserBaseGame(offerId) {
            return GamesDataFactory.getCatalogInfo([offerId])
                .then(selectVaultEdition)
                .then(entitleVaultGame);
        }

        Origin.events.on(Origin.events.CLIENT_NAV_RENEW_SUBSCRIPTION, renewSubscription);

        return {
            buy: buy,
            gift: gift,
            buyAddons: buyAddons,
            entitleVaultGame: entitleVaultGame,
            entitleFreeGame: entitleFreeGame,
            renewSubscription: renewSubscription,
            entitleVaultGameFromLesserBaseGame: entitleVaultGameFromLesserBaseGame,
            subscriptionCheckout: subscriptionCheckout,
            verifyTrialStatus: verifyTrialStatus
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.PurchaseFactory
     * @description  Implements purchase related actions
     */
    angular.module('origin-components')
        .factory('PurchaseFactory', PurchaseFactory);
}());