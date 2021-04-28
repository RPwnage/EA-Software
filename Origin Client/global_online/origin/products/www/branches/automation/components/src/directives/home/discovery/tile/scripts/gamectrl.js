/**
 * @file home/discovery/tile/gamectrl.js
 */
(function() {
    'use strict';

    function OriginHomeDiscoveryTileGameCtrl(
        $scope,
        $location,
        $state,
        $timeout,
        UtilFactory,
        GamesDataFactory,
        UrlHelper,
        ComponentsLogFactory,
        contextNamespace,
        customDescriptionFunction,
        DialogFactory,
        ProductFactory,
        PdpUrlFactory,
        OgdUrlFactory
    ) {

        var updateDownloadStateForOfferId = null,
            friendsTextVisible = false,
            friendsPlayingTextVisible = false,
            friendsTileDisplayVisible = false,
            priceVisible = false,
            TELEMETRYPREFIX = 'telemetry-';

        var throttledDigestFunction = UtilFactory.throttle(function() {
            $timeout(function() {
                $scope.$digest();
            }, 0, false);
        }, 1000);

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
            ComponentsLogFactory.error('[OriginHomeDiscoveryTileGameCtrl] GamesDataFactory.getCatalogInfo', error);
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
            //if we don't have an image passed in, fall back on the pack art
            $scope.image = $scope.discoverTileImage || catalogInfo.i18n.packArtLarge;
            $scope.isUsingPackart = !$scope.discoverTileImage;

            $scope.imageStyle = {};
            if ($scope.discoverTileImage){
                $scope.imageStyle['background-image'] = 'url(' + $scope.image + ')';
            }
            if ($scope.discoverTileColor){
                $scope.imageStyle['background-color'] = '#' + $scope.discoverTileColor;
            }

            //if we don't have a game name passed in, use the display name from catalog
            $scope.gamename = $scope.gamename || catalogInfo.i18n.displayName;

            //if we have a custom setter for description passed in use that, otherwise use the default
            if (customDescriptionFunction) {
                customDescriptionFunction(catalogInfo);
            } else {
                defaultDescriptionFunction(catalogInfo);
            }
            updateSupportTextVisible();
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
            var gamesUpdateHandle = null;
            
            if ($scope.offerId) {
                updateDownloadStateForOfferId = updateDownloadState($scope.offerId);
                $scope.isDownloading = GamesDataFactory.isOperationActive($scope.offerId);

                gamesUpdateHandle = GamesDataFactory.events.on('games:update:' + $scope.offerId, updateDownloadStateForOfferId, this);

                $scope.$on('$destroy', function() {
                    if(gamesUpdateHandle) {
                        gamesUpdateHandle.detach();
                    }
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

        function updateSupportTextVisible() {
            friendsTextVisible = $scope.description && ($scope.description.length > 0);
            $scope.supportTextVisible = friendsTextVisible || friendsPlayingTextVisible || friendsTileDisplayVisible || priceVisible;
            throttledDigestFunction();
        }

        function setSupportTextVisible() {
             $scope.supportTextVisible = false;

             $scope.$on('friendsTextVisible', function(event, visible) {
                friendsTextVisible = visible;
                updateSupportTextVisible();
             });

             $scope.$on('friendsPlayingTextVisible', function(event, visible) {
                friendsPlayingTextVisible = visible;
                updateSupportTextVisible();
             });

             $scope.$on('friendsTileDisplayVisible', function(event, visible) {
                friendsTileDisplayVisible = visible;
                updateSupportTextVisible();
             });

             $scope.$on('priceVisible', function(event, visible) {
                priceVisible = visible;
                updateSupportTextVisible();
             });
        }

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

        //set a class in the dom as a marker for telemetry
        $scope.telemetryTrackerClass =  TELEMETRYPREFIX + contextNamespace;

        //send impression telemetry data once dom is loaded
        setTimeout(Origin.telemetry.sendDomLoadedTelemetryEvent, 0);
        
        setSupportTextVisible();

        //Set component specific class to tile
        $scope.contextClass =  contextNamespace;
        
        /**
         * Send telemetry when the cta is clicked
         */
        function onCtaClicked(event, format) {
            var source = Origin.client.isEmbeddedBrowser() ? 'client' : 'web';
            format = format? format: 'live_tile';

            event.stopPropagation();
            Origin.telemetry.sendStandardPinEvent(Origin.telemetry.trackerTypes.TRACKER_MARKETING, 'message', source, 'success', {
                'type': 'in-game',
                'service': 'originx',
                'format': format,
                'client_state': 'foreground',
                'msg_id': $scope.ctid? $scope.ctid: $scope.image,
                'status': 'click',
                'content': _.get($scope, ['$parent', 'description']) || '',
                'destination_name': _.get($scope, ['$parent', '$parent', 'titleStr']) || ''
            });
        }

        // We listen for an event that is emitted by the transcluded cta directive, to trigger telemetry on cta click.
        $scope.$on('cta:clicked', onCtaClicked);

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
