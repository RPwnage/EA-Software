/**
 * @file home/discovery/home/recommended/action/actionctrl.js
 */
(function() {
    'use strict';

    function OriginHomeRecommendedActionCtrl(
        $scope,
        $location,
        $filter,
        GamesDataFactory,
        ComponentsLogFactory,
        ComponentsUrlsFactory,
        UtilFactory,
        customSubtitleAndDescriptionFunction,
        contextNamespace,
        ProductFactory,
        PdpUrlFactory,
        OgdUrlFactory
    ) {

        var updateDownloadStateForOfferId = null;

        /**
         * update the scope property for isDownloading (this will toggle the progress)
         * @param  {string} offerId the offerId of the game
         */
        function updateDownloadState(offerId) {
            return function() {
                //isDownloading is flag to store the download like operation (updating/repairing/downloading)
                $scope.isDownloading = GamesDataFactory.isOperationActive(offerId);
                $scope.$digest();
            };
        }

        /**
         * parse out the catalog info for a particular reponse
         * @param  {string} offerId the offerId of the game
         * @return {object}         the single catalog object for a given offerid
         */
        function parseCatalogInfo(offerId) {
            return function(response) {
                return response[offerId];
            };
        }

        /**
         * logs an error if we fail to retrieve the catalog
         * @param  {Error} error the error object
         */
        function handleCatalogInfoError(error) {
            ComponentsLogFactory.error('[OriginHomeRecommendedActionCtrl] GamesDataFactory.getCatalogInfo', error.stack);
        }

        /**
         * set the subtitle and description (default behavior)
         */
        function defaultSetSubtitleAndDescription() {
            var tokenObj = {
                '%game%': $scope.gamename
            };

            $scope.subtitle = UtilFactory.getLocalizedStr($scope.subtitleRaw, contextNamespace, 'subtitle', tokenObj);
            $scope.description = UtilFactory.getLocalizedStr($scope.descriptionRaw, contextNamespace, 'description', tokenObj);
        }

        /**
         * sets the bindings in the template
         * @param {object} catalogInfo the games catalog info
         */
        function setBindings(catalogInfo) {
            if ($scope.imageRaw) {
                $scope.imageRaw = ComponentsUrlsFactory.getCMSImageUrl($scope.imageRaw);
            }

            $scope.image = $scope.imageRaw || catalogInfo.i18n.packArtLarge;
            $scope.gamename = $scope.gamename || catalogInfo.i18n.displayName;
            $scope.ctaText = UtilFactory.getLocalizedStr($scope.ctaText, contextNamespace, 'ctatext');
            $scope.ctaUrl = UtilFactory.getLocalizedStr($scope.ctaText, contextNamespace, 'ctaurl');

            if(!$scope.pricingOfferId) {
                $scope.pricingOfferId = $scope.offerId;
            }

            //if we have a custom setter for description passed in use that, otherwise use the default
            if (customSubtitleAndDescriptionFunction) {
                customSubtitleAndDescriptionFunction(catalogInfo);
            } else {
                defaultSetSubtitleAndDescription();
            }

            $scope.$digest();
            return catalogInfo;
        }


        /**
         * Is touch disabled on this device
         * @type {Boolean}
         */
        $scope.isTouchDisable = !UtilFactory.isTouchEnabled();


        if($scope.offerId) {
            ProductFactory.get($scope.offerId).attachToScope($scope, 'model');
        }

        /**
         * Link to the selected game
         */
        $scope.navigateToGame = function() {
            if ($scope.offerId) {
                if(GamesDataFactory.getEntitlement($scope.offerId)) {
                    OgdUrlFactory.goToOgd($scope.model);
                } else {
                    PdpUrlFactory.goToPdp($scope.model);
                }
            }
        };

        $scope.touched = function(){
            if ($scope.model) {
                $scope.navigateToGame();
            } else {
                $location.path($scope.ctaUrl);
            }
        };

        /**
         * retrieve the catalog info (from either the server or cache)
         * @return {promise} resolved when the catal`og info request finishes
         */
        function retrieveCatalogInfo() {
            var promise = null;
            if ($scope.offerId) {
                promise = GamesDataFactory.getCatalogInfo([$scope.offerId])
                    .then(parseCatalogInfo($scope.offerId))
                    .catch(handleCatalogInfoError);

            } else {
                //if there's no offer id (as there isn't for some of the offers, lets just return an empty object)
                var emptyObj = {
                    i18n: {
                    }
                };
                promise = Promise.resolve(emptyObj);
            }

            return promise;
        }

        /**
         * set the initial download state and also setup a listener for any changes
         * @return {[type]} [description]
         */
        function setupDownloadStateAndListener() {
            if ($scope.offerId) {
                $scope.isDownloading = GamesDataFactory.isOperationActive($scope.offerId);

                updateDownloadStateForOfferId = updateDownloadState($scope.offerId);
                GamesDataFactory.events.on('games:update:' + $scope.offerId, updateDownloadStateForOfferId, this);

                $scope.$on('$destroy', function() {
                    GamesDataFactory.events.off('games:update:' + $scope.offerId, updateDownloadStateForOfferId);
                });
            }
        }


        //if we have a valid offer id we want to show friends
        if ($scope.offerId) {
            $scope.displayFriends = true;
        }

        /**
         * Clean up after yoself
         * @method onDestroy
         * @return {void}
         */
        function onDestroy() {
            GamesDataFactory.events.off('games:baseGameEntitlementsUpdate', setupActionTileInfo);
        }

        function setupActionTileInfo() {
            retrieveCatalogInfo()
                .then(setBindings)
                .then(setupDownloadStateAndListener);

        }

        $scope.$on('$destroy', onDestroy);
        GamesDataFactory.events.on('games:baseGameEntitlementsUpdate', setupActionTileInfo);
        setupActionTileInfo();

    }
    /**
     * @ngdoc controller
     * @name origin-components.controller:OriginHomeRecommendedActionCtrl
     * @restrict E
     * @element ANY
     * @scope
     */
    angular.module('origin-components')
        .controller('OriginHomeRecommendedActionCtrl', OriginHomeRecommendedActionCtrl);

}());