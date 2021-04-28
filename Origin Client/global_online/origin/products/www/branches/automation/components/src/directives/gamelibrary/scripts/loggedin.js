/**
 * @file gamelibrary/loggedin.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-gamelibrary-loggedin',
        PAGE_CONTENT_CLASSNAME = '.origin-content',
        REDEEM_CODE_MODAL_HEIGHT='590',
        REDEEM_CODE_MODAL_WIDTH='720',
        THROTTLE_DOM_READY_SPEED = 0;

    function OriginGamelibraryLoggedinCtrl($scope, $filter, $stateParams, $timeout, GamesDataFactory, GamesNonOriginFactory, UtilFactory, ComponentsLogFactory, EventsHelperFactory, ObjectHelperFactory, GamesFilterFactory, AuthFactory, AppCommFactory, PageThinkingFactory, LayoutStatesFactory, CacheFactory, GamesUsageFactory) {
        var toArray = ObjectHelperFactory.toArray;
        var handles = [];
        var timeoutPromise = null;

        function orderByDisplayName() {
            $scope.allGames = $filter('orderBy')($scope.allGames, ['displayName', 'offerId'], false);
        }

        function applyFilters() {
            $scope.activeFiltersCount = GamesFilterFactory.countActiveFilters();
            $scope.displayedGames = GamesFilterFactory.applyFilters($scope.allGames);
            $scope.setPageDimensions();
        }

        function generateModels(catalogData) {
            $scope.allGames = catalogData;
            return catalogData;
        }

        function onModelChange() {
            applyFilters();
            AppCommFactory.events.fire('OriginGamelibraryLoggedin:filterApplied');
            $scope.$digest();
        }

        function updateAllGamesExtraProperties() {
            return _.forEach($scope.allGames, GamesFilterFactory.updateGameModelExtraProperties);
        }

        function mergeGameUsage(catalogData, gameUsageArray) {
            var trimmedCatalogData = toArray(catalogData).map(GamesFilterFactory.createGameModel);
            if (_.isArray(gameUsageArray)) {
                _.forEach(trimmedCatalogData, function(catalogEntry) {
                    catalogEntry.lastPlayedTime = 0;
                    for (var i=0, j=gameUsageArray.length; i<j; i++) {
                        if (gameUsageArray[i] && gameUsageArray[i].masterTitleId === catalogEntry.masterTitleId) {
                            catalogEntry.lastPlayedTime = new Date(gameUsageArray[i].timestamp).getTime();
                            break;
                        }
                    }
                });
            }
            return trimmedCatalogData;
        }

        function getModel() {
            var myOfferIds = GamesDataFactory.baseEntitlementOfferIdList();
            PageThinkingFactory.blockUI();
            return Promise
                .all([
                    GamesDataFactory.getCatalogInfo(myOfferIds),
                    GamesUsageFactory.getAllLastPlayedTimes()
                ])
                .then(_.spread(mergeGameUsage))
                .then(generateModels)
                .then(GamesDataFactory.waitForFiltersReady())
                .then(orderByDisplayName);

        }


        function refreshModel(catalogData) {
            return Promise.resolve(catalogData)
                .then(updateAllGamesExtraProperties)
                .then(GamesFilterFactory.hideLesserEditions)
                .then(GamesFilterFactory.calculateCounts)
                .then(onModelChange)
                .then(PageThinkingFactory.unblockUIPromise)
                .catch(function (error) {
                    PageThinkingFactory.unblockUI();
                    ComponentsLogFactory.error('[origin-gamelibrary-loggedin GamesDataFactory.onUpdate] error', error);
                });
        }

        //Cache the result of refreshModel when called.
        var cachedModel = CacheFactory.decorate(getModel);

        function applyFiltersOnCachedModel() {
            return cachedModel().then(refreshModel);
        }

        function onUpdate() {
            //refresh the cache model on update.
            timeoutPromise = $timeout(cachedModel(CacheFactory.refresh(true)).then(refreshModel), THROTTLE_DOM_READY_SPEED, false);
        }

        function setOnlineStatus() {
            $scope.isOnline = AuthFactory.isClientOnline();
        }

        function updateOnlineState() {
            setOnlineStatus();
            $scope.$digest();
        }

        // We don't know whether the user has any games or not until the library is loaded.
        $scope.allGames = {};
        $scope.hasGames = function () {
            return $scope.allGames === undefined || $scope.allGames.length > 0;
        };

        //use the cached model to filter results.
        $scope.applyFilters = applyFiltersOnCachedModel;

        /*
        * Create observable
        */
        LayoutStatesFactory
            .getObserver()
            .attachToScope($scope, 'states');

        LayoutStatesFactory.setEmbeddedFilterpanel(false);

        $scope.toggleFilterpanel = function () {
            LayoutStatesFactory.toggleEmbeddedFilterpanel();
        };

        $scope.localizedStrings = {
            'addGameLabel': UtilFactory.getLocalizedStr($scope.addgamelabel, CONTEXT_NAMESPACE, 'addgamelabel'),
            'fromOriginAccess': UtilFactory.getLocalizedStr($scope.fromoriginaccess, CONTEXT_NAMESPACE, 'fromoriginaccess'),
            'redeemProductCode': UtilFactory.getLocalizedStr($scope.redeemproductcode, CONTEXT_NAMESPACE, 'redeemproductcode'),
            'nonOriginShortcut': UtilFactory.getLocalizedStr($scope.nonoriginshortcut, CONTEXT_NAMESPACE, 'nonoriginshortcut'),
            'filterLabel': UtilFactory.getLocalizedStr($scope.filterlabel, CONTEXT_NAMESPACE, 'filterlabel'),
            'favouritesLabel': UtilFactory.getLocalizedStr($scope.favouriteslabel, CONTEXT_NAMESPACE, 'favouriteslabel'),
            'allOthersLabel': UtilFactory.getLocalizedStr($scope.allotherslabel, CONTEXT_NAMESPACE, 'allotherslabel'),
            'nogamesTitleLabel': UtilFactory.getLocalizedStr($scope.nogamestitle, CONTEXT_NAMESPACE, 'nogamestitle'),
            'nogamesDescriptionLabel': UtilFactory.getLocalizedStr($scope.nogamesdescriptionlabel, CONTEXT_NAMESPACE, 'nogamesdescription')
        };

        // If the "searchString" URI param is specified, then this is a redirect from the global search page.
        if (!!$stateParams.searchString && $stateParams.searchString.length > 0) {

            // If the user has selected the hidden filter we need to keep that while coming from search results page
            // Remove all the filters except the hidden ones to keep results consistent between gamelibrary and search
            if(GamesFilterFactory.getFilter('isHidden').value){
                GamesFilterFactory.dismissAll();
                GamesFilterFactory.setFilterValue('isHidden', true);
            }
            else {
                GamesFilterFactory.dismissAll();
            }

            // Set the displayName filter to the specified search string
            GamesFilterFactory.getFilter('displayName').value = decodeURI($stateParams.searchString);
        } else {
            GamesFilterFactory.getFilter('displayName').value = '';
        }

        $scope.activeFiltersCount = GamesFilterFactory.countActiveFilters();

        //listen for filter panel changes
        handles = [
            AuthFactory.events.on('myauth:clientonlinestatechanged', updateOnlineState),
            //listen for any filter changes
            GamesDataFactory.events.on('filterChanged', applyFiltersOnCachedModel, this),
            //setup a listener for any entitlement updates
            GamesDataFactory.events.on('games:baseGameEntitlementsUpdate', onUpdate, this),
            // listen for NOG refreshes
            GamesNonOriginFactory.events.on('GamesNonOriginFactory:nogsRefreshed', onUpdate, this)
        ];


        //if we already have data, lets use the entitlement info thats already there
        ComponentsLogFactory.log('GameLibrary: checking initialRefreshCompleted');
        if (GamesDataFactory.initialRefreshCompleted()) {
            onUpdate();
        }

        // Determine whether the user is online or offline when they come to the game library
        setOnlineStatus();

        /* Cleanups */
        function onDestroy () {
            $timeout.cancel(timeoutPromise);
            //disconnect listening for events
            EventsHelperFactory.detachAll(handles);
            LayoutStatesFactory.setEmbeddedFilterpanel(false);
        }

        $scope.$on('$destroy', onDestroy);

        // we will listen for an event that is emitted by game tiles for game updates (download, install,....) to update filterpanel
        $scope.$on('gamelibrary:updatefilter', applyFiltersOnCachedModel);

    }


    function originGamelibraryLoggedin(ComponentsConfigFactory, AnimateFactory, SubscriptionFactory, UrlHelper, ComponentsConfigHelper, $compile, $rootScope) {
        function originGamelibraryLoggedinLink (scope, elem) {
            function setGamelibraryHeight () {
                var gameLibrary = angular.element(elem).find('.l-origin-gamelibrary'),
                    filterPanel = gameLibrary.find('.origin-filter-list').eq(0),
                    filterPanelHeight = filterPanel.outerHeight(true), // includes margin, border, padding
                    offset = filterPanel.offset();

                // This arbitrary 30 is to compensate for iOS-specific hardware
                // issues. The system navbar on those devices add height
                // to the viewport.
                if(offset && offset.top && scope.hasGames()) {
                    var desiredHeight = (filterPanelHeight + offset.top + 30) + 'px' || 0;

                    gameLibrary.css({'min-height': desiredHeight});
                }
            }

            scope.setPageDimensions = function () {
                setGamelibraryHeight();
            };

            function onAddGames() {
                scope.isAddGamesMenuVisible = !scope.isAddGamesMenuVisible;
                if (scope.isAddGamesMenuVisible) {
                    angular.element(document)
                        .on('keyup', closeMenu)
                        .on('mouseup', closeMenu);
                } else {
                    closeMenu();
                }
            }

            function onRedeemProductCode() {
                var sourceurl,modal;

                Origin.telemetry.sendMarketingEvent('Game Library', 'Add Game', 'Redeem Product Code');
                if (Origin.client.isEmbeddedBrowser()) {
                    Origin.client.games.redeemProductCode();
                } else {
                    sourceurl = UrlHelper.buildParameterizedUrl(ComponentsConfigHelper.getUrl('redeemCode'), {
                        'gameId':ComponentsConfigHelper.getSetting('gameId').redeemCode,
                        'locale':Origin.locale.locale(),
                        'sourceType':'igo' //The version of the redeem code that fits best in a modal dialog (temporary measure)
                    });
                    modal = angular.element($compile('<origin-iframe-genericmodal type="default" scrollbars="yes" height='+REDEEM_CODE_MODAL_HEIGHT+' width='+REDEEM_CODE_MODAL_WIDTH+' source-url="'+sourceurl+'"></origin-iframe-genericmodal>')($rootScope));
                    angular.element(PAGE_CONTENT_CLASSNAME).append(modal);
                }
                scope.isAddGamesMenuVisible = false;

            }

            function onAddNonOriginShortcut() {
                Origin.telemetry.sendMarketingEvent('Game Library', 'Add Game', 'Non-Origin Shortcut');
                Origin.client.games.addNonOriginGame();
                scope.isAddGamesMenuVisible = false;
            }

            function closeMenu() {
                scope.isAddGamesMenuVisible = false;
                angular.element(document)
                    .off('keyup', closeMenu)
                    .off('mouseup', closeMenu);

                scope.$digest();
            }

            scope.onAddGames = onAddGames;
            scope.onRedeemProductCode = onRedeemProductCode;
            scope.onAddNonOriginShortcut = onAddNonOriginShortcut;
            scope.isAddGamesMenuVisible = false;
            scope.isSubscriptionActive = SubscriptionFactory.isActive();
            scope.isEmbeddedBrowser = Origin.client.isEmbeddedBrowser();

            AnimateFactory.addResizeEventHandler(scope, scope.setPageDimensions, 300);
        }

        return {
            restrict: 'E',
            scope: {
                addgamelabel: '@',
                fromoriginaccess: '@',
                redeemproductcode: '@',
                nonoriginshortcut: '@',
                filterlabel: '@',
                favouriteslabel: '@',
                allotherslabel: '@',
                nogamestitle: '@',
                nogamesdescription: '@'
            },
            link: originGamelibraryLoggedinLink,
            controller: OriginGamelibraryLoggedinCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/loggedin.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryLoggedin
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} addgamelabel label for the add a game dropdown button
     * @param {LocalizedString} fromoriginaccess label for adding a game from origin access (vault) in the add a game submenu
     * @param {LocalizedString} redeemproductcode label for redeeming a game using a product code
     * @param {LocalizedString} nonoriginshortcut label for adding a non origin game
     * @param {LocalizedString} filterlabel the label for the game library filter dropdown button
     * @param {LocalizedString} favouriteslabel the label for the section heading that appears above favorited games
     * @param {LocalizedString} allotherslabel the label for the section that contains games without a favorite flag
     * @param {LocalizedString} nogamestitle the title text for the messaging if a user search fails to find more games
     * @param {LocalizedString} nogamesdescription description text for the messaging if a user search fails to find more games
     *
     * @description
     *
     * game library container
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-gamelibrary-loggedin
     *           addgamelabel="Add Game..."
     *           fromoriginaccess="Origin Access Vault Game"
     *           redeemproductcode="Redeem a Product Code..."
     *           nonoriginshortcut="Non-Origin Game..."
     *           filterlabel="Filter Library"
     *           favouriteslabel="Favorites"
     *           allotherslabel="All Other Games"
     *           nogamestitle="No results found"
     *           nogamesdescription="Try removing or changing some of your filters."
     *         ></origin-gamelibrary-loggedin>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginGamelibraryLoggedinCtrl', OriginGamelibraryLoggedinCtrl)
        .directive('originGamelibraryLoggedin', originGamelibraryLoggedin);

}());
