/**
 * @file gamelibrary/scripts/ogdexpansiontile.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-ogd-expansion-tile',
        PERMISSION_PREFIX_MAP = {'0': '', '1': '(1102) ', '2': '(1103) '};

    function OriginOgdExpansionTileCtrl($scope, DialogFactory, GamesDataFactory, OfferTransformer, UtilFactory, ProductInfoHelper, GamesContextFactory, DateHelperFactory, SubscriptionFactory, moment, EventsHelperFactory, GiftingFactory) {
        $scope.releaseDateLoc = UtilFactory.getLocalizedStr($scope.releasedatestr, CONTEXT_NAMESPACE, 'releasedatestr');
        $scope.freeLoc = UtilFactory.getLocalizedStr($scope.freestr, CONTEXT_NAMESPACE, 'freestr');
        $scope.unlockedLoc = UtilFactory.getLocalizedStr($scope.unlockedstr, CONTEXT_NAMESPACE, 'unlockedstr');
        $scope.installedLoc = UtilFactory.getLocalizedStr($scope.installedstr, CONTEXT_NAMESPACE, 'installedstr');
        $scope.upgradeLoc = UtilFactory.getLocalizedStr($scope.upgradestr, CONTEXT_NAMESPACE, 'upgradestr');
        $scope.readMoreLoc = UtilFactory.getLocalizedStr($scope.readmorestr, CONTEXT_NAMESPACE, 'readmorestr');
        $scope.onWishlistLoc = UtilFactory.getLocalizedStr($scope.onWishlistStr, CONTEXT_NAMESPACE, 'on-wishlist');
        $scope.viewWishlistLoc = UtilFactory.getLocalizedStr($scope.viewWishlistStr, CONTEXT_NAMESPACE, 'view-wishlist');

        $scope.isInTheFuture = DateHelperFactory.isInTheFuture;
        $scope.usingSmallPackArt = false;
        $scope.modelLoaded = false;
        $scope.model = {};
        $scope.isPrivateOffer = false;

        var isAvailableFromSubscription = false,
            longDescElement,
            longDescContainer,
            eventHandles;

        $scope.isFree = function(model) {
            return ProductInfoHelper.isFree(model);
        };

        $scope.isDownloadable = function(model) {
            return model.downloadable && ProductInfoHelper.isDownloadable(model);
        };

        $scope.isPurchaseAction = function() {
            return (!$scope.model.isOwned || $scope.model.vaultEntitlement) && !ProductInfoHelper.isFree($scope.model) ? true : false;
        };

        $scope.isReleased = function(model) {
            return DateHelperFactory.isInThePast(model.releaseDate);
        };

        $scope.isGiftable = function(model) {
            return GiftingFactory.isGameGiftable(model);
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

        // set entitlement cta text
        $scope.freeCtaLoc = function() {
            return (isSubscriptionOffer() && SubscriptionFactory.userHasSubscription()) ? $scope.upgradeLoc : $scope.freeLoc;
        };

        $scope.showLongDescriptionDialog = function() {
            DialogFactory.openAlert({
                id: 'origin-ogd-expansion-tile-show-long-description',
                title: $scope.model.displayName,
                description: $scope.model.longDescription,
                rejectText: UtilFactory.getLocalizedStr($scope.closestr, CONTEXT_NAMESPACE, 'closestr')
            });
        };

        this.setElements = function(el) {
            longDescElement = el.find('.origin-gamecard-long-description');
            longDescContainer = el.find('.origin-gamecard-long-description-container');
        };

        $scope.showReadMore = function() {
            return (longDescElement.height() > longDescContainer.height());
        };

        $scope.showLongDescription = function() {
            if (!!$scope.model.longDescription && !$scope.model.mediumDescription) {
                return ($scope.model.longDescription.length && ($scope.model.longDescription !== $scope.model.shortDescription));
            }
            return false;
        };

        /**
         * format a given date
         * @param {Date} JS date object to be formatted
         * @return {string} formatted date
         * @method formatDate         
         */
        $scope.formatDate = function(d) {
            return moment(d).format('LL');
        };

        function setIsAvailableFromSubscription(available) {
            isAvailableFromSubscription = available;
        }

        function setPrimaryAction(action) {
            $scope.primaryAction = action;
        }

        function setModel(data) {
            if (!_.isUndefined(data) && !_.isUndefined(data[$scope.offerId])) {
                $scope.model = data[$scope.offerId];                                  
            }
        }

        function checkPrivateOffers() {
            var entitlement = GamesDataFactory.getEntitlement($scope.offerId);
            $scope.isPrivateOffer = GamesDataFactory.isPrivate(entitlement);            
        }

        function existsNotDefault(packArtUrl) {
            return (!!packArtUrl && packArtUrl.length > 0 && packArtUrl !== GamesDataFactory.defaultBoxArtUrl());
        }

        function getPackArtUrl() {
            return _.filter([$scope.model.i18n.packArtLarge, $scope.model.i18n.packArtMedium, $scope.model.i18n.packArtSmall], existsNotDefault)[0];
        }

        function setPackArt() {
            $scope.expansionPackArt = getPackArtUrl(); 
            $scope.usingSmallPackArt = ($scope.expansionPackArt === $scope.model.i18n.packArtSmall);
        }

        function setDebugInfo() {
            var permissions = _.get(GamesDataFactory.getEntitlement($scope.offerId), 'originPermissions') || '0',
                prefix = PERMISSION_PREFIX_MAP[permissions] || '';
            $scope.debugInfo = prefix + $scope.offerId;
        }

        function loadModel() {
            Promise.all([
                GamesContextFactory.primaryAction($scope.offerId, true)
                    .then(setPrimaryAction),
                GamesDataFactory.isAvailableFromSubscription($scope.offerId)
                    .then(setIsAvailableFromSubscription),
                GamesDataFactory.getCatalogInfo([$scope.offerId])
                    .then(OfferTransformer.transformOffer)
                    .then(setModel)
                    .then(checkPrivateOffers)
                    .then(setPackArt)
                    .then(setDebugInfo)
                ]).then(function() {
                    $scope.modelLoaded = true;
                    $scope.$digest();
                });
        }
        loadModel();

        function handleGameUpdate() {
            loadModel();
        }
        eventHandles = [GamesDataFactory.events.on('games:update:' + $scope.offerId, handleGameUpdate)];

        function onDestroy() {
            EventsHelperFactory.detachAll(eventHandles);
        }

        // when the element is destroyed make sure that we cleanup
        $scope.$on('$destroy', onDestroy);

    }

    function originOgdExpansionTile(ComponentsConfigFactory) {
        function expansionTileLinkFn(scope, element, attrs, ctrl) {
            ctrl.setElements(element);
        }
        var directiveDefinitionObj = {
            restrict: 'E',
            replace: true,
            scope: {
                offerId: '@',
                releasedatestr: '@',
                unlockedstr: '@',
                installedstr: '@',
                freestr: '@',
                upgradestr: '@',
                readmorestr: '@',
                closestr: '@',
                onWishlistStr: '@onwishliststr',
                viewWishlistStr: '@viewwishliststr',
                showDebugInfo: '='
            },
            link: expansionTileLinkFn,
            controller: OriginOgdExpansionTileCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/ogdexpansiontile.html')
        };
        return directiveDefinitionObj;
    }



    /**
     * @ngdoc directive
     * @name origin-components.directives:originOgdExpansionTile
     * @restrict E
     * @element ANY
     * @scope
     * @param {String} offer Id
     * @param {LocalizedString} releasedatestr Release Date
     * @param {LocalizedString} unlockedstr You own this content
     * @param {LocalizedString} freestr FREE Get it now
     * @param {LocalizedString} installedstr Installed
     * @param {LocalizedString} upgradestr Upgrade
     * @param {LocalizedString} readmorestr Read More
     * @param {LocalizedString} closestr *Close
     * @param {LocalizedString} onwishliststr "On your wishlist"
     * @param {LocalizedString} viewwishliststr "View full wishlist"
     * @param {Boolean} show-debug-info Debug info will be shown if true
     *
     * @description Used on OGD page to display individual expansion tiles
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-ogd-expansion-tile upgradestr="Upgrade" installedstr="Installed" unlockedstr="Unlocked" freestr="Free" releasedatestr="Release Date" readmorestr="Read More" offer-id="OFR.123"></origin-ogd-expansion-tile>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginOgdExpansionTileCtrl', OriginOgdExpansionTileCtrl)
        .directive('originOgdExpansionTile', originOgdExpansionTile);
}());
