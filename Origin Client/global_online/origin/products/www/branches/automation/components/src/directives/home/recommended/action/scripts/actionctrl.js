/**
 * @file home/discovery/home/recommended/action/actionctrl.js
 */
(function() {
    'use strict';

    function OriginHomeRecommendedActionCtrl(
        $scope,
        $location,
        $filter,
        $timeout,
        $sce,
        GamesDataFactory,
        ComponentsLogFactory,
        UrlHelper,
        UtilFactory,
        customSubtitleAndDescriptionFunction,
        contextNamespace,
        ProductFactory,
        PdpUrlFactory,
        OgdUrlFactory
    ) {

        var updateDownloadStateForOfferId = null,
            friendsTextVisible = false,
            friendsPlayingTextVisible = false,
            friendsTileDisplayVisible = false,
            TELEMETRY_PREFIX = 'origin-telemetry-rna-';

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
         * logs an error if we fail to retrieve the catalog
         * @param  {Error} error the error object
         */
        function handleError(error) {
            ComponentsLogFactory.error('[OriginHomeRecommendedActionCtrl]', error);
        }

        /**
         * Get the game name token
         * @return {Object} the game name replacement
         */
        function getGameNameToken() {
            return {
                '%game%': $scope.gamename
            };
        }

        /**
         * set the subtitle and description (default behavior)
         */
        function defaultSetSubtitleAndDescription() {
            var gameNameToken = getGameNameToken();

            $scope.subtitle = UtilFactory.getLocalizedStr($scope.subtitleRaw, contextNamespace, 'subtitle', gameNameToken);
            $scope.description = UtilFactory.getLocalizedStr($scope.descriptionRaw, contextNamespace, 'description', gameNameToken);
        }

        /**
         * sets the bindings in the template
         * @param {object} catalogInfo the games catalog info
         */
        function setBindings(catalogInfo) {
            //grab the image taking into account CQ Defaults
            $scope.discoverTileImage = UtilFactory.getLocalizedStr($scope.discoverTileImage, contextNamespace, 'discover-tile-image') ;

            //if we grab a string back that doesn't start with http (i.e. missdiscover-tile-image) don't treat that as a valid URL
            if(!_.startsWith($scope.discoverTileImage, 'http')) {
                $scope.discoverTileImage = null;
            }

            $scope.image = $scope.discoverTileImage || catalogInfo.i18n.packArtLarge;
            $scope.backupImage = !$scope.discoverTileImage;
            $scope.gamename = $scope.gamename || catalogInfo.i18n.displayName;
            $scope.ctaText = UtilFactory.getLocalizedStr($scope.ctaText, contextNamespace, 'ctatext');
            $scope.ctaUrl = UtilFactory.getLocalizedStr($scope.ctaUrl, contextNamespace, 'ctaurl');

            //set the title string on the header
            $scope.sectionTitle = $sce.trustAsHtml(UtilFactory.getLocalizedStr($scope.sectionTitle, contextNamespace, 'section-title', getGameNameToken()));
            $scope.sectionSubtitle = $sce.trustAsHtml(UtilFactory.getLocalizedStr($scope.sectionSubtitle, contextNamespace, 'section-subtitle', getGameNameToken()));

            if (!$scope.priceOfferId) {
                $scope.priceOfferId = $scope.offerId;
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
         * Link to the selected game
         */
        $scope.navigateToGame = function() {
            if ($scope.offerId) {
                if (GamesDataFactory.getEntitlement($scope.offerId)) {
                    OgdUrlFactory.goToOgd($scope.model);
                } else {
                    PdpUrlFactory.goToPdp($scope.model);
                }
            }
        };

        $scope.touched = function() {
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
        function retrieveCatalogInfoIfAvailable() {
            var emptyObj = {
                    i18n: {}
            };
            return $scope.offerId ? GamesDataFactory.getCatalogInfo([$scope.offerId]) : Promise.resolve([emptyObj]);
        }

        /**
         * set the initial download state and also setup a listener for any changes
         * @return {[type]} [description]
         */
        function setupDownloadStateAndListener() {
            var gamesUpdateHandle = null;

            if ($scope.offerId) {
                $scope.isDownloading = GamesDataFactory.isOperationActive($scope.offerId);

                updateDownloadStateForOfferId = updateDownloadState($scope.offerId);
                gamesUpdateHandle = GamesDataFactory.events.on('games:update:' + $scope.offerId, updateDownloadStateForOfferId, this);

                $scope.$on('$destroy', function() {
                    if(gamesUpdateHandle) {
                        gamesUpdateHandle.detach();
                    }
                });
            }
        }

        function setIds(ids) {
            $scope.offerId = ids[0] || $scope.offerId;
            $scope.priceOfferId = ids[1] || $scope.priceOfferId;

            return [$scope.offerId];
        }

        function setModel(response) {
            $scope.model = _.first(_.values(response));
            return $scope.model;
        }

        function setDisplayFriends() {
            $scope.displayFriends = !!$scope.offerId;
        }

        function nonDirectiveSupportTextVisible() {
            return ($scope.subtitle && $scope.subtitle.length > 0) ||
                (!$scope.displayFriends && $scope.description && $scope.description.length > 0) ||
                ($scope.showTrialViolator) ||
                ($scope.priceOfferId && $scope.displayPrice);
        }

        function updateSupportTextVisible() {
            $scope.supportTextVisible = friendsTextVisible || friendsPlayingTextVisible || friendsTileDisplayVisible || nonDirectiveSupportTextVisible();
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
        }

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
                'content': $scope.description,
                'destination_name': $sce.valueOf($scope.sectionTitle)
            });
        }

        // We listen for an event that is emitted by the transcluded cta directive, to send cta-click telemetry for this tile.
        $scope.$on('cta:clicked', onCtaClicked);

        /**
         * Is touch disabled on this device
         * @type {Boolean}
         */
        $scope.isTouchDisable = !UtilFactory.isTouchEnabled();

        //set a class in DOM as a marker for Telemetry
        $scope.telemetryTrackerClass = TELEMETRY_PREFIX + contextNamespace;

        //set tile stype in DOM as marker for Telemetry
        $scope.telemetryTrackerType = contextNamespace;

        //send impression telemetry data once dom is loaded
        setTimeout(Origin.telemetry.sendDomLoadedTelemetryEvent, 0);

        setSupportTextVisible();

        Promise.all([
            GamesDataFactory.lookUpOfferIdIfNeeded($scope.ocdPath, $scope.offerId),
            GamesDataFactory.lookUpOfferIdIfNeeded($scope.priceOcdPath, $scope.priceOfferId)
        ])
            .then(setIds)
            .then(retrieveCatalogInfoIfAvailable)
            .then(setModel)
            .then(setBindings)
            .then(setupDownloadStateAndListener)
            .then(setDisplayFriends)
            .then(updateSupportTextVisible)
            .catch(handleError);

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
