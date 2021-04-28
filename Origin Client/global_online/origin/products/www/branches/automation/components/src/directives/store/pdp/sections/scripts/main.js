/**
 * @file store/pdp/sections/scripts/main.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-pdp-hero',
        LEGAL_CONTEXT = 'pdp',
        NOT_FOUND_PAGE_ROUTE = 'app.error_notfound',
        DESCRIPTION_CHAR_LIMIT = 190;


    function OriginStorePdpSectionsMainCtrl($location, $scope, $stateParams, AppCommFactory, AuthFactory, DateHelperFactory, DirectiveScope, GamesCatalogFactory, GamesDataFactory, ObjectHelperFactory, OcdPathFactory, OcdPathFactoryConstant, OfferTransformer, OriginCheckoutTypesConstant, QueueFactory, PdpUrlFactory, PurchaseFactory, UserObserverFactory, UtilFactory, $filter, GameRefiner, NavigationFactory, PdpFactory, ScopeHelper, GiftingWizardFactory) {
        // flag that prevents ODC auto checkout from opening multiple instances of checkout
        var autoCheckoutStarted = false;
        
        $scope.model = {};
        $scope.goToPdp = PdpUrlFactory.goToPdp;
        $scope.getPdpUrl = PdpUrlFactory.getPdpUrl;

        UserObserverFactory.getObserver($scope, 'user');
        $scope.editions = [];
        $scope.getLocalizedEdition = UtilFactory.getLocalizedEditionName;

        $scope.lesserEditionInVault = function() {
            if($scope.vaultEdition && GameRefiner.isBaseGameOfferType($scope.model)) {
                return $scope.vaultEdition.weight < $scope.model.weight;
            }

            return false;
        };

        $scope.greaterEditionInVault = function() {
            if($scope.vaultEdition && GameRefiner.isBaseGameOfferType($scope.model)) {
                return $scope.vaultEdition.weight > $scope.model.weight && $scope.model.isUpgradeable;
            }

            return false;
        };

        $scope.goToOriginAccessLandingPage = function ($event) {
            $event.preventDefault();
            AppCommFactory.events.fire('uiRouter:go', 'app.store.wrapper.origin-access');
        };

        $scope.accessLandingUrl = NavigationFactory.getAbsoluteUrl('app.store.wrapper.origin-access');


        function getHighestOwnedBaseGameOfferId(offerId){
            var offer = GamesCatalogFactory.getExistingCatalogInfo(offerId);
            if(!GameRefiner.isBaseGame(offer)) {
                var baseGame = GamesDataFactory.getBaseGameEntitlementByMasterTitleId(_.get(offer, 'masterTitleId'));
                offerId = (_.get(baseGame, 'offerId')) || _.get($scope, ['baseGameModel', 'offerId']);
            }
            return offerId;
        }
        /**
         * Redirects user to the OGD page for the current product
         * redirects to highest owned basegame for dlc/addon/xpac
         * @return {void}
         */
        $scope.goToLibrary = function (offerId, $event) {
            $event.preventDefault();
            offerId = getHighestOwnedBaseGameOfferId(offerId);
            AppCommFactory.events.fire('uiRouter:go', 'app.game_gamelibrary.ogd', {
                offerid: offerId
            });
        };

        $scope.getLibraryUrl = function(offerId){
            offerId = getHighestOwnedBaseGameOfferId(offerId);
            return NavigationFactory.getAbsoluteUrl('app.game_gamelibrary.ogd', {
                offerid: offerId
            });
        };

        $scope.ownsLesserEdition = function() {
            return PdpFactory.ownsLesserEdition($scope.editions, $scope.model);
        };

        $scope.ownsGreaterEdition = function() {
            return PdpFactory.ownsGreaterEdition($scope.editions, $scope.model);
        };

        $scope.ownsGreaterEditionNotVault = function() {
            return PdpFactory.ownsGreaterEditionNotVault($scope.editions, $scope.model);
        };

        $scope.ownsGreaterOrEqualEditionNotVault = function() {
            return PdpFactory.ownsGreaterOrEqualEditionNotVault($scope.editions, $scope.model);
        };

        $scope.isReleased = function() {
            if($scope.model && $scope.model.releaseDate) {
                return DateHelperFactory.isInThePast($scope.model.releaseDate);
            } else {
                return false;
            }
        };

        $scope.updateModel = function(model) {
            $scope.model = model;
        };

        $scope.resetModel = function() {
            $scope.model = $scope.selectedModel;
        };

        $scope.isEarlyAccessTrialReleased = function() {
            if($scope.earlyAccessTrialModel) {
                return DateHelperFactory.isInThePast($scope.earlyAccessTrialModel.releaseDate);
            } else {
                return false;
            }
        };

        $scope.hasManualOverrides = function() {
            return $scope.manualTrialOverride || $scope.manualDiscountOverride ? true : false;
        };

        // sort siblings and build edition list based on type
        // note: basegames and bundles should be considered the same type
        function chooseOtherEditions(offer) {
            if(GameRefiner.isBaseGameOfferType(offer)) {
                return !GameRefiner.isAlphaTrialDemoOrBeta(offer);
            } else {
                return offer.isPurchasable && offer.type === $scope.model.type && !GameRefiner.isAlphaTrialDemoOrBeta(offer);
            }
        }

        function choosePurchasableEditions(offer) {
            if(GameRefiner.isBaseGameOfferType(offer)) {
                return offer.isPurchasable && !GameRefiner.isAlphaTrialDemoOrBeta(offer);
            } else {
                return offer.isPurchasable && offer.type === $scope.model.type && !GameRefiner.isAlphaTrialDemoOrBeta(offer);
            }
        }

        function sortEditions(a, b) {
            var aWeight = _.get(a, 'weight', 0);
            var bWeight = _.get(b, 'weight', 0);
            var aReleaseDate = _.get(a, 'releaseDate');
            var bReleaseDate = _.get(b, 'releaseDate');

            // sort by weight if they are not weighted the same
            if(aWeight !== bWeight) {
                return bWeight - aWeight;
            } else if(GameRefiner.isCurrency(a)) {
                // sort currency by price ascending
                var aPrice = _.get(a, 'price', 0);
                var bPrice = _.get(b, 'price', 0);

                return aPrice - bPrice;
            } else if(_.isDate(aReleaseDate) && _.isDate(bReleaseDate) && aReleaseDate.valueOf() !== bReleaseDate.valueOf()) {
                // sort everything else by release date descending
                return bReleaseDate.valueOf() - aReleaseDate.valueOf();
            } else {
                // if all else fails, dont modify sort
                return 0;
            }
        }

        function getEditions(data) {
            var purchasableEditions = _.filter(data, choosePurchasableEditions);
            $scope.editions = _.filter(data, chooseOtherEditions);

            if(_.size(purchasableEditions) > 1) {
                $scope.purchasableEditions = purchasableEditions.sort(sortEditions);
            }
        }

        // find the purchasable vault edition. this should only happen on DLC that
        // are included in a hidden shell bundle (sims 3)
        function chooseVaultShellBundle(catalogData) {
            OfferTransformer
                .transformOffer(_.filter(PdpFactory.unwrapEditions(catalogData), {vault: true, isPurchasable: true}))
                .then(setVaultEdition);
        }

        // check that the vault edition we found is purchsable, if its not, it might be a 
        // hidden shell bundle and we need to get it's purchsable version, so go get all
        // of its siblings and look for the vault edition in those offers
        function setVaultEdition(data) {
            // if there is one vault game found, we need to check if its purchsable,
            // if it is not, we need to find the purchsable subling
            if(_.size(data) === 1) {
                var vaultEdition = _.first(_.values(data));

                // if we found 1 vault edition and its purchsable, we are not on a shell bundles
                // PDP, we need to set the purchsable version to the $scope for entitlement actions
                if(_.get(vaultEdition, 'isPurchasable', false)) {
                    $scope.vaultEdition = vaultEdition;
                    $scope.$evalAsync();
                } else if(_.size(_.get(vaultEdition, 'siblings', [])) > 0) {
                    // if its not purchasable, go get this offers siblings and try and find it
                    var promises = [];

                    _.forEach(vaultEdition.siblings, function(siblingPath) {
                        promises.push(GamesDataFactory.getCatalogByPath(siblingPath));
                    });

                    Promise
                        .all(promises)
                        .then(chooseVaultShellBundle);
                }
            // if there are 2 vault editions, this is a hidden shell bundle, we need to get 
            // the purchasable one to link to, but not grant the entitlement. this is used on
            // DLC in hidden shell bundles PDP's to link to the vault games PDP
            } else if(_.size(data) === 2) {
                $scope.vaultEdition = _.first(_.filter(_.values(data), {isPurchasable: false}));
                $scope.$evalAsync();
            }
            // if there is no vault edition found, this is not a vault game. do not set $scope.vaultEdition
        }

        // get the vault edition from current pdps siblings, if the vault edition is not
        // included in the siblings, go get it. we need OCD data here incase the vault edition
        // is a hidden shell bundle
        function getVaultEdition(data) {
            var vaultEdition = _.filter(data, {subscriptionAvailable: true});

            // is the vault edition included in this PDPs siblings
            if(!_.isEmpty(vaultEdition)) {
                setVaultEdition(vaultEdition);
            } else if(_.get($scope, ['model', 'vaultOcdPath'], false)) {
                OcdPathFactory
                    .promiseGet(_.get($scope, ['model', 'vaultOcdPath']))
                    .then(setVaultEdition);
            }
        }

        function setBaseGames(data) {
            $scope.baseGameModel = _.first(_.values(data));

            $scope.$evalAsync();
        }

        function getBaseGames() {
            GamesDataFactory
                .getLowestRankedPurchsableBasegame($scope.selectedModel)
                .then(ObjectHelperFactory.wrapInArray)
                .then(GamesDataFactory.getCatalogInfo)
                .then(OfferTransformer.transformOffer)
                .then(setBaseGames);
        }

        function handleOtherEditions(data) {

            // build edition selector list
            if(GameRefiner.isBaseGameOfferType($scope.model) || GameRefiner.isCurrency($scope.model)) {
               getEditions(data);
            }

            getVaultEdition(data);

            if(GameRefiner.isBaseGameOfferType($scope.model)) {
                $scope.earlyAccessTrialModel = _.first(_.filter(data, {earlyAccess: true}));
                $scope.trialModels = _.filter(data, {trial: true});
                $scope.demoModels = _.filter(data, {demo: true});
                $scope.betaModels = _.filter(data, {beta: true});
            }

            if(!GameRefiner.isBaseGame($scope.model) && !GameRefiner.isBaseGameOfferType($scope.model) && $scope.model.type !== 'freegames') {
                getBaseGames();
            }
            // editions need to trigger digest or they don't show up
            ScopeHelper.digestIfDigestNotInProgress($scope);
        }

        function automaticCheckout(offerId) {
            var invoiceSourceGameId = $stateParams.invoiceSource,
                buyParams = {
                    checkoutType: OriginCheckoutTypesConstant.DEFAULT,
                    invoiceSource: invoiceSourceGameId
                };

            if (_.isString(invoiceSourceGameId)) {
                // once we have started the automatic checkout, remove invoice source from url so we don't trigger it again
                $location.search('invoiceSource', null);
                AuthFactory.promiseLogin()
                    .then(function () {
                        PurchaseFactory.buy(offerId, buyParams);
                    });
            }
        }

        function automaticGiftCheckout(game) {
            var invoiceSourceGameId = $stateParams.giftInvoiceSource;

            if (_.isString(invoiceSourceGameId)) {
                // once we have started the automatic checkout, remove invoice source from url so we don't trigger it again
               setTimeout(function() {
                   GiftingWizardFactory.loginAndStartGiftingFlow(game);
               }, 500);
               $location.search('giftInvoiceSource', null);
            }
        }

        function automaticAddToWishlist(offerId) {
            var invoiceSourceGameId = $stateParams.wishlistInvoiceSource;

            if (_.isString(invoiceSourceGameId)) {
                // once we have started the automatic checkout, remove invoice source from url so we don't trigger it again

                if ($scope.showCollapseBtn()) {
                    $scope.collapseState = false;
                }

                QueueFactory.enqueue('addOfferToWishlistODC', {offerId: offerId});

                $location.search('wishlistInvoiceSource', null);

            }
        }

        function navigateToPdp(ocdPath) {
            if (ocdPath) {
                var params = ocdPath.split('/'),
                    parameters = {
                        franchise: params[1],
                        game: params[2],
                        edition: params[3]
                    };

                AppCommFactory.events.fire('uiRouter:go', 'app.store.wrapper.pdp', parameters);
            }
        }
        /**
         * Gets the models of siblings and find the first one that is valid and redirect to the PDP of that sibling
         * @param model
         */
        function navigateToFirstAvailableSibling(model){
            PdpFactory.getFirstAvailableSibling(model).then(function(model){
                PdpUrlFactory.goToPdp(model);
            }, function(){
                NavigationFactory.goToState(NOT_FOUND_PAGE_ROUTE);
            });
            
        }

        function applyModel(data) {
            var model = _.first(_.values(data));

            // 404 as nothing was found
            if(_.size(data) === 0) {
                NavigationFactory.goToState(NOT_FOUND_PAGE_ROUTE);
                return;
            }

            // offer data is still loading the observable, just return and wait
            if(_.size(model) === 0) {
                return;
            }

            // offer is not purchasable and has no siblings
            if((!_.get(model, 'isPurchasable', false) && _.size(_.get(model, 'siblings', [])) === 0)) {
                //based on ORSTOR-2173
                //If all offers are de-activated and there is no 'retirement' page, redirect to Browse Games page
                AppCommFactory.events.fire('uiRouter:go', 'app.store.wrapper.browse', {}, {location: false});
                return;
            }

            // no offer, or offer is not purchasable and has siblings
            if ((model.error === OcdPathFactoryConstant.offerNotFoundError || !_.get(model, 'isPurchasable', false)) && _.size(_.get(model, 'siblings', [])) > 0) {
                navigateToFirstAvailableSibling(model);
                return;
            }

            $scope.model = model;
            $scope.selectedModel = model;

            if ($scope.model.offerId && !autoCheckoutStarted) {
                autoCheckoutStarted = true;

                automaticCheckout($scope.model.offerId);
                automaticGiftCheckout($scope.model);
                automaticAddToWishlist($scope.model.offerId);
            }

            // redirect trials, demos, betas, alphas, limited trials and early access trials to basegame pdps
            if($scope.model.freeBaseGame) {
                navigateToPdp($scope.model.freeBaseGame);
            }

            if(_.size($scope.model.siblings) > 0) {
                PdpFactory.getEditions($scope.model)
                    .then(handleOtherEditions);
            } else if(_.get($scope, ['model', 'subscriptionAvailable'], false)) {
                getVaultEdition([$scope.model]);
            }

            $scope.truncatedLongDescription = $filter('limitHtml')($scope.model.longDescription, DESCRIPTION_CHAR_LIMIT, '...');
        }

        function getModel() {
            $scope.ocdDataReady = false;
            $scope.mainStrings = {};
            $scope.platformStrings = {};

            OcdPathFactory.get(_.compact([PdpUrlFactory.buildPathFromUrl()])).attachToScope($scope, applyModel);
            DirectiveScope.populateScope($scope, CONTEXT_NAMESPACE, PdpUrlFactory.buildPathFromUrl()).then(function() {
                $scope.platformStrings.platformText = UtilFactory.getLocalizedStr($scope.platformText, CONTEXT_NAMESPACE, 'platform-text');
                $scope.mainStrings.purchaseSeperately = UtilFactory.getLocalizedStr($scope.purchaseSeperately, CONTEXT_NAMESPACE, 'purchase-seperately');

                $scope.ocdDataReady = true;
            });
        }

        // get catalog and OCD data for this PDP
        getModel();

        GamesDataFactory.events.once('games:extraContentEntitlementsUpdate', getModel);

        $scope.collapseState = true;

        $scope.collapsable = function () {
            return $scope.user.isSubscriber &&
                    ($scope.model.subscriptionAvailable || $scope.model.isUpgradeable) &&
                    !$scope.model.oth;
        };

        $scope.collapsed = function() {
            return $scope.user.isSubscriber &&
                    !$scope.model.isOwned &&
                    ($scope.model.subscriptionAvailable || $scope.model.isUpgradeable) &&
                    !$scope.model.oth;
        };

        $scope.showCollapseBtn = function() {
            return $scope.user.isSubscriber &&
                    (!$scope.model.isOwned || $scope.model.vaultEntitlement) &&
                    ($scope.model.subscriptionAvailable || $scope.model.isUpgradeable) &&
                    !$scope.model.oth ? true : false;
        };

        $scope.hideCollapseArea = function() {
            return $scope.user.isSubscriber &&
                    !$scope.model.oth &&
                    (!$scope.model.isOwned || $scope.model.vaultEntitlement) &&
                    ($scope.model.subscriptionAvailable || $scope.model.isUpgradeable) &&
                    $scope.collapsed &&
                    $scope.collapsable;
        };

        $scope.context = LEGAL_CONTEXT;

        function detachExtraContentEntitlementUpdateEvent(){
            GamesDataFactory.events.off('games:extraContentEntitlementsUpdate', getModel);
        }

        $scope.$on('$destroy', detachExtraContentEntitlementUpdateEvent);

        $scope.handleChange = function(changedOffer) {
            var newOfferId = _.get(changedOffer, 'offerId'),
                oldOfferId = _.get($scope, ['selectedModel', 'offerId']);

            if(!_.isUndefined(newOfferId) && !_.isUndefined(oldOfferId) && newOfferId !== oldOfferId) {
                PdpUrlFactory.goToPdp(changedOffer);
            }
        };
    }

    function originStorePdpSectionsMain(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            controller: 'OriginStorePdpSectionsMainCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/sections/views/main.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpSectionsMain
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-pdp-sections-main />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStorePdpSectionsMainCtrl', OriginStorePdpSectionsMainCtrl)
        .directive('originStorePdpSectionsMain', originStorePdpSectionsMain);
}());
