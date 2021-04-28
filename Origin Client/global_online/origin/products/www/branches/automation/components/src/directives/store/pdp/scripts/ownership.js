/**
 * @file store/pdp/ownership.js
 */
/* global moment */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-pdp-hero';

    function OriginStorePdpOwnershipCtrl($scope, OwnershipFactory, UtilFactory, ProductInfoHelper, GamesEntitlementFactory, SubscriptionFactory, GameRefiner, DateHelperFactory) {
        function setOwnedEdition(ownedEdition) {
            $scope.ownedEdition = ownedEdition;

            return ownedEdition;
        }

        function getOwnedEdition() {
            if(GameRefiner.isBaseGameOfferType($scope.model) && !_.isEmpty($scope.editions)) {
                return Promise.resolve(OwnershipFactory.getTopOwnedEdition($scope.editions));
            } else if(_.get($scope, ['model', 'isOwned'])) {
                return Promise.resolve($scope.model);
            } else {
                return Promise.resolve(null);
            }
        }

        function updateOwnedEditionMessages() {
            var gameName = _.get($scope.ownedEdition, 'displayName');
            $scope.ownershipStrings.vaultEditionOwnedBySubscriber = UtilFactory.getLocalizedStr($scope.vaultEditionOwnedBySubscriber, CONTEXT_NAMESPACE, 'vault-edition-owned-by-subscriber', {'%game%': gameName});

            var preloadDate = moment(_.get($scope.ownedEdition, 'downloadStartDate')).format('LL');
            $scope.ownershipStrings.preloadMessage = UtilFactory.getLocalizedStr($scope.preLoadText, CONTEXT_NAMESPACE, 'pre-load-text', {'%date%': preloadDate, '%preloaddate%': preloadDate});

            // mouse out of edition selectors needs to trigger digest
            $scope.$evalAsync();
        }

        getOwnedEdition().then(setOwnedEdition).then(updateOwnedEditionMessages).catch(_.partial(setOwnedEdition, null));

        $scope.ownershipStrings = {
            viewInLibrary: UtilFactory.getLocalizedStr($scope.viewInLibrary, CONTEXT_NAMESPACE, 'view-in-library'),
            preLoadedTextMessage: UtilFactory.getLocalizedStr($scope.preLoadedTextMessage, CONTEXT_NAMESPACE, 'preloaded-text-message')
        };

        $scope.isBundle = function() {
            return $scope.ownedEdition ? GameRefiner.isBundle($scope.ownedEdition) : false;
        };

        $scope.isPreloadable = function() {
            return $scope.ownedEdition ? ProductInfoHelper.isPreloadable($scope.ownedEdition) : false;
        };

        function checkPreloadDate() {
            if ($scope.ownedEdition && _.isDate(_.get($scope, ['ownedEdition', 'downloadStartDate'], null))) {
                var now = moment(),
                    preloadDate = moment($scope.ownedEdition.downloadStartDate);
                return Math.round(preloadDate.diff(now, 'days', true)) >= 365;
            } else {
                // if there is no date, it's to far away to display.
                return true;
            }
        }

        $scope.isPreloaded = function() {
            return $scope.ownedEdition ? ProductInfoHelper.isInstalled($scope.ownedEdition) : false;
        };

        $scope.isDownloadable = function() {
            return $scope.ownedEdition ? ProductInfoHelper.isDownloadable($scope.ownedEdition) : false;
        };

        $scope.getOwnedMessage = function(owned) {
            if (owned) {
                if (owned.editionName && GameRefiner.isBaseGame(owned)) {
                    var parameters = {
                        '%edition%': owned.editionName
                    };

                    if(new Date($scope.ownedEdition.releaseDate).valueOf() >= new Date().valueOf()) {
                        return UtilFactory.getLocalizedStr($scope.youPreorderedText, CONTEXT_NAMESPACE, 'you-preordered-text', parameters);
                    } else {
                        return UtilFactory.getLocalizedStr($scope.youOwnText, CONTEXT_NAMESPACE, 'you-own-text', parameters);
                    }
                } else {
                    return UtilFactory.getLocalizedStr(undefined, CONTEXT_NAMESPACE, 'youownthis');
                }
            } else {
                return '';
            }
        };

        function updateMessages() {
            getOwnedEdition().then(setOwnedEdition).then(updateOwnedEditionMessages).catch(_.partial(setOwnedEdition, null));

            $scope.ownershipStrings.preorderMessage = UtilFactory.getLocalizedStr($scope.preOrderText, CONTEXT_NAMESPACE, 'pre-order-text', {'%date%': ''});
            $scope.ownershipStrings.preorderReleaseDate = _.get($scope, ['model', 'i18n', 'preAnnouncementDisplayDate']) || DateHelperFactory.formatDateWithTimezone($scope.model.releaseDate);
            $scope.preloadDateTooFarAway = checkPreloadDate();
        }

        function init() {
            updateMessages();
            GamesEntitlementFactory.events.on('GamesEntitlementFactory:getEntitlementSuccess', updateMessages);
            SubscriptionFactory.events.on('SubscriptionFactory:subscriptionUpdate', updateMessages);
            $scope.$watchCollection('editions', updateMessages);
            $scope.$watchCollection('model', updateMessages);
        }

        var stopWatchingOcdDataReady = $scope.$watch('ocdDataReady', function (isOcdDataReady) {
            if (isOcdDataReady) {
                stopWatchingOcdDataReady();
                init();
            }
        });

        function onDestroy() {
            GamesEntitlementFactory.events.off('GamesEntitlementFactory:getEntitlementSuccess', updateMessages);
            SubscriptionFactory.events.off('SubscriptionFactory:subscriptionUpdate', updateMessages);
        }

        $scope.$on('$destroy', onDestroy);
    }

    function originStorePdpOwnership(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/ownership.html'),
            controller: 'OriginStorePdpOwnershipCtrl'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpOwnership
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offer-id product offer ID
     * @description
     *
     * PDP product ownership indicator
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-ownership></origin-store-pdp-ownership>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .controller('OriginStorePdpOwnershipCtrl', OriginStorePdpOwnershipCtrl)
        .directive('originStorePdpOwnership', originStorePdpOwnership);
}());
