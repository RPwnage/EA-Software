/**
 * @file gamelibrary/loggedin.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-gamelibrary-loggedin';

    function OriginGamelibraryLoggedinCtrl($scope, $filter, GamesDataFactory, UtilFactory, ComponentsLogFactory, ObjectHelperFactory, GamesFilterFactory, AuthFactory) {
        var orderService = $filter('orderBy');
        var toArray = ObjectHelperFactory.toArray;
        var takeHead = ObjectHelperFactory.takeHead;

        function createGameModel(catalogData) {
            var model = {
                offerId: catalogData.offerId,
                displayName: catalogData.i18n.displayName,
                platforms: Object.keys(catalogData.platforms)
            };

            return model;
        }

        function orderByDisplayName() {
            $scope.allGames = orderService($scope.allGames, 'displayName', false);
        }

        function applyFilters() {
            $scope.activeFiltersCount = GamesFilterFactory.countActiveFilters();
            $scope.displayedGames = GamesFilterFactory.applyFilters($scope.allGames);
        }

        function generateModels(catalogData) {
            $scope.allGames = toArray(catalogData).map(createGameModel);
            $scope.isLoading = false;
        }

        function populateModelProperties(model) {
            model.isPcGame = model.platforms.indexOf('PCWIN') >= 0;
            model.isMacGame = model.platforms.indexOf('MAC') >= 0;

            model.isFavorite = GamesDataFactory.isFavorite(model.offerId);
            model.isHidden = GamesDataFactory.isHidden(model.offerId);

            model.isPlayedLastWeek = false; //for now always return false
            model.isNonOrigin = GamesDataFactory.isNonOriginGame(model.offerId);
            model.isSubscription = GamesDataFactory.isSubscriptionEntitlement(model.offerId);
            model.isDownloading = GamesDataFactory.isDownloading(model.offerId);
            model.isInstalled = GamesDataFactory.isInstalled(model.offerId);

            return model;
        }

        function onModelChange() {
            angular.forEach($scope.allGames, populateModelProperties);
            GamesFilterFactory.calculateCounts($scope.allGames);
            applyFilters();
            $scope.$digest();
        }

        function onUpdate() {
            var myOfferIds = GamesDataFactory.baseEntitlementOfferIdList();

            Promise
                .all([
                    GamesDataFactory.getCatalogInfo(myOfferIds),
                    GamesDataFactory.waitForFiltersReady()
                ])
                .then(takeHead)
                .then(generateModels)
                .then(orderByDisplayName)
                .then(onModelChange)
                .catch(function(error) {
                    ComponentsLogFactory.error('[origin-gamelibrary-loggedin GamesDataFactory.onUpdate] error', error.message);
                });
        }

        function setOnlineStatus() {
            $scope.isOnline = AuthFactory.isClientOnline();
        }

        function updateOnlineState() {
            setOnlineStatus();
            $scope.$digest();
        }

        // We don't know whether the user has any games or not until the library is loaded.
        $scope.isLoading = true;
        $scope.allGames = {};
        $scope.hasGames = function () {
            return $scope.allGames === undefined || $scope.allGames.length > 0;
        };

        $scope.applyFilters = applyFilters;

        $scope.isFilterPanelOpen = false;
        $scope.toggleFilterpanel = function () {
            $scope.isFilterPanelOpen = !$scope.isFilterPanelOpen;
        };

        $scope.localizedStrings = {
            'favouritesLabel': UtilFactory.getLocalizedStr($scope.favouritesLabel, CONTEXT_NAMESPACE, 'favouriteslabel'),
            'allOthersLabel': UtilFactory.getLocalizedStr($scope.allOthersLabel, CONTEXT_NAMESPACE, 'allotherslabel'),
            'hiddenLabel': UtilFactory.getLocalizedStr($scope.hiddenLabel, CONTEXT_NAMESPACE, 'hiddenlabel'),
            'addGameLabel': UtilFactory.getLocalizedStr($scope.addGameLabel, CONTEXT_NAMESPACE, 'addgamelabel'),
            'filterLabel': UtilFactory.getLocalizedStr($scope.filterLabel, CONTEXT_NAMESPACE, 'filterlabel')
        };

        $scope.activeFiltersCount = 0;

        $scope.$on('$destroy', function() {
            //disconnect listening for events
            GamesDataFactory.events.off('games:baseGameEntitlementsUpdate', onUpdate);
            GamesDataFactory.events.off('filterChanged', onModelChange);
            AuthFactory.events.off('myauth:clientonlinestatechanged', updateOnlineState);
        });

        AuthFactory.events.on('myauth:clientonlinestatechanged', updateOnlineState);

        //listen for any filter changes
        GamesDataFactory.events.on('filterChanged', onModelChange);

        //setup a listener for any entitlement updates
        GamesDataFactory.events.on('games:baseGameEntitlementsUpdate', onUpdate, this);

        //if we already have data, lets use the entitlement info thats already there
        ComponentsLogFactory.log('GameLibrary: checking initialRefreshCompleted');
        if (GamesDataFactory.initialRefreshCompleted()) {
            onUpdate();
        }

        // Determine whether the user is online or offline when they come to the game library
        setOnlineStatus();
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryLoggedin
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} favourites title for the 'favourites' games list
     * @param {LocalizedString} allothers title for the 'all others' games list
     * @param {LocalizedString} hidden title for the 'all others' games list
     * @param {LocalizedString} addgame text for the 'Add Game' page navigation item
     * @param {LocalizedString} filter text for the 'Filter' page navigation item
     * @description
     *
     * game library container
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-gamelibrary-loggedin></origin-gamelibrary-loggedin>
     *     </file>
     * </example>
     *
     */
    function originGamelibraryLoggedin(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                favouritesLabel: '@favourites',
                allOthersLabel: '@allothers',
                hiddenLabel: '@hidden',
                addGameLabel: '@addgame',
                filterLabel: '@filter'
            },
            controller: 'OriginGamelibraryLoggedinCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/loggedin.html')
        };
    }

    angular.module('origin-components')
        .controller('OriginGamelibraryLoggedinCtrl', OriginGamelibraryLoggedinCtrl)
        .directive('originGamelibraryLoggedin', originGamelibraryLoggedin);

}());
