/**
 * @file game/library/tile.js
 */
(function() {

    'use strict';

    /* jshint ignore:start */
    /**
     * BooleanEnumeration
     * @enum {string}
     */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    var CONTEXT_NAMESPACE = 'origin-game-tile';

    function OriginGameTileCtrl($scope, $timeout, $stateParams, UtilFactory, GamesDataFactory, GamesContextFactory, EventsHelperFactory, SubscriptionFactory, EntitlementStateRefiner, BeaconFactory, ComponentsLogFactory, OwnedGameActionHelper, AppCommFactory, NavigationFactory) {

        var DOUBLE_CLICK_TIMEOUT = 250, // if we get a click event, wait 250 millisec in case this is a double-click
            myEvents = new Origin.utils.Communicator(),
            catalogInfoReady = false,
            handles,
            catalogInfo,
            entitlementInfo,
            isValidPlatform,
            isTrialExpired,
            clientInfo,
            stringLookup = {
                'startdownloadcta': $scope.startDownloadCtaStr,
                'pausedownloadcta': $scope.pauseDownloadCtaStr,
                'resumedownloadcta': $scope.resumeDownloadCtaStr,
                'canceldownloadcta': $scope.cancelDownloadCtaStr,
                'viewgamedetailscta': $scope.viewGameDetailsCtaStr,
                'playcta': $scope.playCtaStr,
                'starttrialcta': $scope.startTrialCtaStr,
                'checkforupdatescta': $scope.checkForUpdatesCtaStr,
                'pauseupdatecta': $scope.pauseUpdateCtaStr,
                'resumeupdatecta': $scope.resumeUpdateCtaStr,
                'cancelupdatecta': $scope.cancelUpdateCtaStr,
                'checkforrepairgamecta': $scope.checkForRepairGameCtaStr,
                'pauserepaircta': $scope.pauseRepairCtaStr,
                'resumerepaircta': $scope.resumeRepairCtaStr,
                'cancelrepaircta': $scope.cancelRepairCtaStr,
                'installcta': $scope.installCtaStr,
                'uninstallcta': $scope.uninstallCtaStr,
                'openinodtcta': $scope.openInOdtCtaStr,
                'updatecta': $scope.updateCtaStr,
                'pausegamecta': $scope.pauseGameCtaStr,
                'cancelgamecta': $scope.cancelGameCtaStr,
                'resumegamecta': $scope.resumeGameCtaStr,
                'addtodownloadscta': $scope.addToDownloadsCtaStr,
                'downloadnowcta': $scope.downloadNowCtaStr,
                'updatenowcta': $scope.updateNowCtaStr,
                'repairnowcta': $scope.repairNowCtaStr,
                'download': $scope.downloadStr,
                'preload': $scope.preloadStr,
                'cancelcloudsynccta': $scope.cancelCloudSyncCtaStr,
                'repaircta': $scope.repairCtaStr,
                'restorecta': $scope.restoreCtaStr,
                'pauseinstallcta': $scope.pauseInstallCtaStr,
                'resumeinstallcta': $scope.resumeInstallCtaStr,
                'cancelinstallcta': $scope.cancelInstallCtaStr,
                'showgamedetailscta': $scope.showGameDetailsCtaStr,
                'addasfavoritecta': $scope.addAsFavoriteCtaStr,
                'removeasfavoritecta': $scope.removeAsFavoriteCtaStr,
                'updategamecta': $scope.updateGameCtaStr,
                'customizeboxartcta': $scope.customizeBoxArtCtaStr,
                'gamepropertiescta': $scope.gamePropertiesCtaStr,
                'removefromlibrarycta': $scope.removeFromLibraryCtaStr,
                'hidecta': $scope.hideCtaStr,
                'viewachievements': $scope.viewAchievementsStr,
                'unhidecta': $scope.unhideCtaStr
            },
            delayLoadedPromise;

        $scope.delayLoaded = false;

        /**
         * The digest cycle is extremely expensive with violators
         * so delay loading the violators until a few ms after
        */
        function enableDelayLoaded() {

            function showDelayedItems() {
                delayLoadedPromise = null;
                $scope.delayLoaded = true;
                $scope.$digest();
            }

            function show() {
                var offerIdArray = _.map(GamesDataFactory.getExtraContentEntitlementsByMasterTitleId(catalogInfo.masterTitleId), 'offerId');
                //we make sure we have the catalog info for all the entitlements associated with this
                //tile before we enable/show the violators
                //
                if (offerIdArray.length) {
                    GamesDataFactory.getCatalogInfo(offerIdArray)
                        .then(showDelayedItems)
                        .catch(showDelayedItems);
                } else {
                    showDelayedItems();
                }
            }

            if (catalogInfoReady) {
                show();
            }
            else {
                myEvents.once('tile:catalogInfoReady', show);
            }
        }

        /**
         * Determine if the game is currently in download queue
         * @return {Boolean}
         * @method determineIfDownloading
         */
        function isInOperationQueue() {
            return (clientInfo &&
                clientInfo.progressInfo &&
                (clientInfo.progressInfo.active || clientInfo.completedIndex > -1));
        }

        /**
        * Determine if the game tile should be grayed out
        * @return {Boolean}
        * @method isDisabled
        */
        function isDisabled() {
            return EntitlementStateRefiner.isDisabled(catalogInfo, entitlementInfo, clientInfo, isValidPlatform, isTrialExpired, SubscriptionFactory.isActive());
        }

        /**
        * Determine if the game tile should be highlighted
        * @return {Boolean}
        * @method isHighlighted
        */
        function isHighlighted() {
            return (clientInfo && clientInfo.isSelectedInODT);
        }

        /**
         * Stores the product information to the scope after
         * doing some transforms to the clientActions
         * @return {void}
         * @method storeProductInfo
         */
        function storeGameData(response) {
            catalogInfo = response[0][$scope.offerId];
            catalogInfoReady = true;
            myEvents.fire('tile:catalogInfoReady');
            entitlementInfo = response[1];
            clientInfo = response[2];
            isValidPlatform = response[3];
            isTrialExpired = response[4];
        }

        /**
        * Store the tile data
        * @method setTileData
        */
        function setTileData() {
            $scope.packArt = catalogInfo.i18n.packArtLarge;
            $scope.defaultPackArt = GamesDataFactory.defaultBoxArtUrl();
            $scope.displayName = catalogInfo.i18n.displayName;
            $scope.isDisabled = GamesDataFactory.isNonOriginGame($scope.offerId) ? false : isDisabled();
            $scope.isQueued = isInOperationQueue();
            $scope.isHighlighted = isHighlighted();

            // Disable hover for iOS devices
            if (UtilFactory.isTouchEnabled()) {
                $scope.showDetailsButton = false;
            }
            $scope.$digest();
        }

        /**
        * When the catalog gets updated update the catalog info
        * and then update the tile data
        * @method onCatalogUpdated
        */
        function onCatalogUpdated(data) {
            catalogInfo = _.merge(catalogInfo, data);
            setTileData();
        }

        /**
         * If the game is downloading then make sure we show the game info
         * @return {void}
         * @method updateDownloadState
         */
        function updateDownloadState() {
            // emit a signal for game library so that filters can be updated
            $scope.$emit('gamelibrary:updatefilter');
            $scope.isQueued = isInOperationQueue();
            $scope.$digest();
        }

        /**
        * When the client sends a game update event, make sure we reflect the new state
        * @method onGameUpdated
        */
        function onGameUpdated() {
            $scope.isHighlighted = isHighlighted();
            updateDownloadState();
        }

        /**
         * Translates labels in the given list of menu item objects
         * @return {object}
         */
        function translateMenuStrings(menuItems) {
            angular.forEach(menuItems, function (item) {
                item.label = UtilFactory.getLocalizedStr(stringLookup[item.label], CONTEXT_NAMESPACE, item.label);
            });
            return menuItems;
        }

        /**
        * Subscribe to all of the events
        * @method initEvents
        */
        function initEvents() {
            handles = [
                GamesDataFactory.events.on('games:update:' + $scope.offerId, onGameUpdated),
                GamesDataFactory.events.on('games:downloadqueueupdate:' + $scope.offerId, updateDownloadState),
                GamesDataFactory.events.on('games:catalogUpdated:' + $scope.offerId, onCatalogUpdated)
            ];
            $scope.$on('$destroy', function() {
                EventsHelperFactory.detachAll(handles);
            });
        }

        /**
        * Handle an error
        * @param {Error} error
        * @method onError
        */
        function onError(error) {
            ComponentsLogFactory.error('[origin-game-tile] error', error);
        }

        /**
         * Generates list of context menu items available for this game
         * @return {object}
         */
        $scope.getAvailableActions = function() {
            if (angular.isUndefined($scope.showContextMenu) || $scope.showContextMenu === true) {
                return GamesContextFactory
                    .contextMenu($scope.offerId)
                    .then(translateMenuStrings);
            }
            else {
                // We don't want to show a context menu, so return a null-resolved promise in order to indicate that 
                // there aren't any context menu items, to disable the context menu directive.
                return Promise.resolve(null);
            }
        };

        /**
         * Goes to respectful OGD page. Removes item from complete list.
         * @return {object}
         */
        $scope.cancelClick = false;
        $scope.clicked = false;

        $scope.onTileClick = function () {
            if ($scope.clicked) {
                $scope.cancelClick = true;
                return;
            }

            $scope.clicked = true;
            GamesDataFactory.showGameDetailsPageForOffer($scope.offerId);

            $timeout(function() {
                if ($scope.cancelClick) {
                    $scope.cancelClick = false;
                    $scope.clicked = false;
                    return;
                }

                if (angular.isDefined(clientInfo) && clientInfo.completedIndex > -1) {
                    Origin.client.contentOperationQueue.remove($scope.offerId);
                }

                // Record telemetry for game tile click on game library page only.  Because this game tile is also
                // used on the search results page, check directive parameters to ensure that this is a game library
                // tile.
                if (!$scope.showDetailsButton) {
                    Origin.telemetry.sendMarketingEvent('Game Library', 'Product', $scope.offerId);
                }

                $scope.cancelClick = false;
                $scope.clicked = false;
            }, DOUBLE_CLICK_TIMEOUT, false);

        };

        /**
         * On tile double-click, launch the game
         */
        $scope.onTileDblClick = function () {
            AppCommFactory.events.fire('uiRouter:go', 'app.game_gamelibrary');
            OwnedGameActionHelper.getItWithOrigin($scope.offerId);
        };

        /* Get Translated Strings */
        $scope.strings = {
            detailsButtonLabel: UtilFactory.getLocalizedStr($scope.detailsctaStr, CONTEXT_NAMESPACE, 'detailscta')
        };

        /**
         * Go to Game Details Page using "details" button click. Only valid from search results page as its using the showdetailsbutton
         * @return {void}
         */
        $scope.goToGameDetailsPage = function($event) {
            $event.preventDefault();
            if(!_.isEmpty($stateParams.searchString)){
                AppCommFactory.events.fire('uiRouter:go', 'app.game_gamelibrary.ogd', {
                    offerid: $scope.offerId,
                    searchString: decodeURI($stateParams.searchString)
                });
            } else {
                AppCommFactory.events.fire('uiRouter:go', 'app.game_gamelibrary.ogd', {
                    offerid: $scope.offerId
                });
            }
        };

        $scope.getGameDetailsPageUrl = function(){
            if(!_.isEmpty($stateParams.searchString)){
                return NavigationFactory.getAbsoluteUrl('app.game_gamelibrary.ogd', {
                    offerid: $scope.offerId,
                    searchString: decodeURI($stateParams.searchString)
                });
            } else {
                return NavigationFactory.getAbsoluteUrl('app.game_gamelibrary.ogd',  {
                    offerid: $scope.offerId
                });
            }
        };

        // disable when you are not on the same platform
        // or if your subscription has expired
        $scope.isDisabled = false;
        $scope.isQueued = isInOperationQueue();

        Promise.all([
            GamesDataFactory.getCatalogInfo([$scope.offerId]),
            GamesDataFactory.getEntitlement($scope.offerId),
            GamesDataFactory.getClientGamePromise($scope.offerId),
            BeaconFactory.isInstallable(),
            GamesDataFactory.isTrialExpired($scope.offerId)
        ])
            .then(storeGameData)
            .then(setTileData)
            .then(initEvents)
            .catch(onError);



        function onDestroy() {
            if(delayLoadedPromise) {
                $timeout.cancel(delayLoadedPromise);
                delayLoadedPromise = null;
            }
        }

        $scope.$on('$destroy', onDestroy);

        delayLoadedPromise = $timeout(enableDelayLoaded, 300, false);
    }

    function originGameTile(ComponentsConfigFactory) {

        return {
            restrict: 'E',
            scope: {
                offerId: '@offerid',
                hoursPlayed: '@hoursplayed',
                showDetailsButton: '@showdetailsbutton',
                detailsctaStr: '@detailscta',
                showContextMenu: '@showcontextmenu',
                startDownloadCtaStr: '@startdownloadcta',
                pauseDownloadCtaStr: '@pausedownloadcta',
                resumeDownloadCtaStr: '@resumedownloadcta',
                cancelDownloadCtaStr: '@canceldownloadcta',
                viewGameDetailsCtaStr: '@viewgamedetailscta',
                playCtaStr: '@playcta',
                startTrialCtaStr: '@starttrialcta',
                checkForUpdatesCtaStr: '@checkforupdatescta',
                pauseUpdateCtaStr: '@pauseupdatecta',
                resumeUpdateCtaStr: '@resumeupdatecta',
                cancelUpdateCtaStr: '@cancelupdatecta',
                checkForRepairGameCtaStr: '@checkforrepairgamecta',
                pauseRepairCtaStr: '@pauserepaircta',
                resumeRepairCtaStr: '@resumerepaircta',
                cancelRepairCtaStr: '@cancelrepaircta',
                installCtaStr: '@installcta',
                uninstallCtaStr: '@uninstallcta',
                openInOdtCtaStr: '@openinodtcta',
                updateCtaStr: '@updatecta',
                pauseGameCtaStr: '@pausegamecta',
                cancelGameCtaStr: '@cancelgamecta',
                resumeGameCtaStr: '@resumegamecta',
                addToDownloadsCtaStr: '@addtodownloadscta',
                downloadNowCtaStr: '@downloadnowcta',
                updateNowCtaStr: '@updatenowcta',
                repairNowCtaStr: '@repairnowcta',
                downloadStr: '@download',
                preloadStr: '@preload',
                cancelCloudSyncCtaStr: '@cancelcloudsynccta',
                repairCtaStr: '@repaircta',
                restoreCtaStr: '@restorecta',
                pauseInstallCtaStr: '@pauseinstallcta',
                resumeInstallCtaStr: '@resumeinstallcta',
                cancelInstallCtaStr: '@cancelinstallcta',
                showGameDetailsCtaStr: '@showgamedetailscta',
                addAsFavoriteCtaStr: '@addasfavoritecta',
                removeAsFavoriteCtaStr: '@removeasfavoritecta',
                updateGameCtaStr: '@updategamecta',
                customizeBoxArtCtaStr: '@customizeboxartcta',
                gamePropertiesCtaStr: '@gamepropertiescta',
                removeFromLibraryCtaStr: '@removefromlibrarycta',
                hideCtaStr: '@hidecta',
                viewAchievementsStr: '@viewachievements',
                unhideCtaStr: '@unhidecta'
            },
            controller: 'OriginGameTileCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/views/tile.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameTile
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offerid the offerid of the game you want to interact with
     * @param {number} hoursplayed hours of game play
     * @param {BooleanEnumeration} showdetailsbutton should we show the details cta?
     * @param {LocalizedString} detailscta details cta button label
     * @param {BooleanEnumeration} showcontextmenu should we enable or allow the context menu to be shown?
     * @param {LocalizedString} startdownloadcta "Download"
     * @param {LocalizedString} pausedownloadcta "Pause Download"
     * @param {LocalizedString} resumedownloadcta "Resume Download"
     * @param {LocalizedString} canceldownloadcta "Cancel Download"
     * @param {LocalizedString} viewgamedetailscta "Game Details"
     * @param {LocalizedString} playcta "Play"
     * @param {LocalizedString} starttrialcta "Start Trial"
     * @param {LocalizedString} checkforupdatescta "Check for Updates"
     * @param {LocalizedString} pauseupdatecta "Pause Update"
     * @param {LocalizedString} resumeupdatecta "Resume Update"
     * @param {LocalizedString} cancelupdatecta "Cancel Update"
     * @param {LocalizedString} checkforrepairgamecta "Check For Repair Game"
     * @param {LocalizedString} pauserepaircta "Pause Repair"
     * @param {LocalizedString} resumerepaircta "Resume Repair"
     * @param {LocalizedString} cancelrepaircta "Cancel Repair"
     * @param {LocalizedString} installcta "Install"
     * @param {LocalizedString} uninstallcta "Uninstall"
     * @param {LocalizedString} openinodtcta "Open in ODT"
     * @param {LocalizedString} updatecta "Update"
     * @param {LocalizedString} pausegamecta "Pause %game%"
     * @param {LocalizedString} cancelgamecta "Cancel %game%"
     * @param {LocalizedString} resumegamecta "Resume %game%"
     * @param {LocalizedString} addtodownloadscta "Add To Downloads"
     * @param {LocalizedString} downloadnowcta "Download Now"
     * @param {LocalizedString} updatenowcta "Update Now"
     * @param {LocalizedString} repairnowcta "Repair Now"
     * @param {LocalizedString} download "Download"
     * @param {LocalizedString} preload "Pre-load"
     * @param {LocalizedString} preloadcta Context menu text for starting Pre-load
     * @param {LocalizedString} pausepreloadcta Context menu text for pausing Pre-load
     * @param {LocalizedString} resumepreloadcta Context menu text for resuming Pre-load
     * @param {LocalizedString} cancelpreloadcta Context menu text for cancelling Pre-load
     * @param {LocalizedString} cancelcloudsynccta "Cancel Cloud Sync"
     * @param {LocalizedString} repaircta "Repair"
     * @param {LocalizedString} restorecta "Restore"
     * @param {LocalizedString} pauseinstallcta "Pause Install"
     * @param {LocalizedString} resumeinstallcta "Resume Install"
     * @param {LocalizedString} cancelinstallcta "Cancel Install"
     * @param {LocalizedString} showgamedetailscta "Game Details"
     * @param {LocalizedString} addasfavoritecta "Add to Favorites"
     * @param {LocalizedString} removeasfavoritecta "Remove from Favorites"
     * @param {LocalizedString} updategamecta "Update Game"
     * @param {LocalizedString} customizeboxartcta "Customize Box Art..."
     * @param {LocalizedString} gamepropertiescta "Game Properties"
     * @param {LocalizedString} removefromlibrarycta "Remove from Library"
     * @param {LocalizedString} hidecta "Hide from Library"
     * @param {LocalizedString} viewachievementscta "View Achievements"
     * @param {LocalizedString} unhidecta "Unhide"
     * @param {LocalizedString} viewspecialoffercta "View Special Offer"
     * @param {LocalizedString} viewinstorecta "View in Store"
     *
     * @description
     *
     * game tile
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-tile
     *             offerid="OFB-EAST:109549060"
     *             hoursplayed="4"
     *             showdetailsbutton="true"
     *             detailsctastr="More Details"
     *             showcontextmenu="true"
     *         ></origin-game-tile>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginGameTileCtrl', OriginGameTileCtrl)
        .directive('originGameTile', originGameTile);
}());
