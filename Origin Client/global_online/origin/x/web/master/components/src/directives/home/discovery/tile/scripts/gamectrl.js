/**
 * @file home/discovery/tile/gamectrl.js
 */
(function() {
    'use strict';

    function OriginHomeDiscoveryTileGameCtrl(
        $scope,
        $location,
        $state,
        UtilFactory,
        GamesDataFactory,
        ComponentsUrlsFactory,
        ComponentsLogFactory,
        contextNamespace,
        customDescriptionFunction,
        DialogFactory,
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
         * parse out the catalog info for a particular response
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
            ComponentsLogFactory.error('[OriginHomeDiscoveryTileGameCtrl] GamesDataFactory.getCatalogInfo', error.stack);
        }

        /**
         * set the description (default behavior)
         */
        function defaultDescriptionFunction() {
            var tokenObj = {
                '%game%': $scope.gamename
            };
            $scope.description = UtilFactory.getLocalizedStr($scope.descriptionRaw, contextNamespace, 'description', tokenObj);
        }

        /**
         * sets the bindings in the template
         * @param {object} catalogInfo the games catalog info
         */
        function setBindings(catalogInfo) {
            //if we have an image url passed in we need to append the CQ origin to it
            if ($scope.imageRaw) {
                $scope.imageRaw = ComponentsUrlsFactory.getCMSImageUrl($scope.imageRaw);
            }

            //if we don't have an image passed in, fall back on the pack art
            $scope.image = $scope.imageRaw || catalogInfo.i18n.packArtLarge;

            //if we don't have a game name passed in, use the display name from catalog
            $scope.gamename = $scope.gamename || catalogInfo.i18n.displayName;

            //if we have a custom setter for description passed in use that, otherwise use the default
            if (customDescriptionFunction) {
                customDescriptionFunction(catalogInfo);
            } else {
                defaultDescriptionFunction(catalogInfo);
            }

            $scope.$digest();
        }

        /**
         * retrieve the catalog info (from either the server or cache)
         * @return {promise} resolved when the catalog info request finishes
         */
        function retrieveCatalogInfo() {
            var promise = null;
            if ($scope.offerId) {
                promise = GamesDataFactory.getCatalogInfo([$scope.offerId])
                    .then(parseCatalogInfo($scope.offerId))
                    .catch(handleCatalogInfoError);

            } else {
                promise = Promise.resolve({});
            }

            return promise;
        }

        /**
         * set the initial download state and also setup a listener for any changes
         * @return {[type]} [description]
         */
        function setupDownloadStateAndListener() {
            if ($scope.offerId) {
                updateDownloadStateForOfferId = updateDownloadState($scope.offerId);
                $scope.isDownloading = GamesDataFactory.isOperationActive($scope.offerId);

                GamesDataFactory.events.on('games:update:' + $scope.offerId, updateDownloadStateForOfferId, this);

                $scope.$on('$destroy', function() {
                    GamesDataFactory.events.off('games:update:' + $scope.offerId, updateDownloadStateForOfferId);
                });
            }
        }

        /*
         * Attach the product Model to the scope.
         */
        if($scope.offerId) {
            // TODO Some tiles aren't getting any information about the product
            // this data needs to come from the feed so we can link to the product
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

        /**
         * Is touch disabled on this device
         * @type {Boolean}
         */
        $scope.isTouchDisable = !UtilFactory.isTouchEnabled();

        /**
         * Exectued when the tile is touched or clicked on a touch enabled device.
         *
         * @return {void}
         */
        $scope.touched = function() {
            DialogFactory.openAlert({
                id: 'web-going-to-?',
                title: 'Going to ???',
                description: 'When the rest of OriginX is fully functional, you will be taken to the ???',
                rejectText: 'OK'
            });
        };

        //starting point for the controller
        retrieveCatalogInfo()
            .then(setBindings)
            .then(setupDownloadStateAndListener);
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileGameController
     * @restrict E
     * @element ANY
     * @scope
     */
    angular.module('origin-components')
        .controller('OriginHomeDiscoveryTileGameCtrl', OriginHomeDiscoveryTileGameCtrl);

}());