(function () {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-search-page',
        PEOPLE_RESULTS_PER_PAGE = 20;

    /**
     * Global Search Resutls Page Ctrl
     */
    function OriginSearchPageCtrl($scope, $state, $stateParams, SearchFactory, ComponentsLogFactory, AuthFactory, UtilFactory, EventsHelperFactory) {

        var start = 0,
            handles = [];

        $scope.searchingLoc = UtilFactory.getLocalizedStr($scope.searchingStr, CONTEXT_NAMESPACE, 'searching');

        /**
         * Shows all of the people search results and hides other search category results
         * @return {void}
         * @method showAllPeopleCallback
         */
        $scope.showAllPeopleCallback = function() {

            Origin.telemetry.sendTelemetryEvent('TRACKER_DEV', 'search_view_all', 'user', 'search_view_all', { 'category': 'people' });

            $scope.isGamesVisible = false;
            $scope.isStoreVisible = false;
            $scope.peopleResultsLimit = 'Infinity';
            $scope.showingAllPeople  = true;

            window.scrollTo(0, 0);

            $state.go($state.current, {
                searchString: getSearchString(),
                category: 'people'
            });
        };

        /**
         * Handle Game Search Results
         * @return search results
         * @method handleSearchGamesResults
         */
        function handleSearchGamesResults(games) {

            if(games.length) {
                $scope.gameResults = games;
                $scope.isGamesVisible = true;
            }
            $scope.$digest();

            return games;
        }

        /**
         * Handle Store and People Search Results
         * @return {Object}
         * @method handleStoreAndPeopleResults
         */
        function handleStoreAndPeopleResults(resultSet) {

            if(resultSet.store !== undefined){
                $scope.storeResults = resultSet.store;
                $scope.isStoreVisible = true;
            }

            if(resultSet.people !== undefined){
                $scope.peopleResults = resultSet.people;
                $scope.isPeopleVisible = true;
            }
            $scope.$digest();

            return resultSet;
        }

        /**
         * Handle more People Search Results
         * @return {void}
         * @method handleMorePeopleResults
         */
        function handleMorePeopleResults(results) {

            if(!_.isEmpty(results)) {
                start += PEOPLE_RESULTS_PER_PAGE;
                $scope.peopleResults.users.push.apply($scope.peopleResults.users, results.infoList);
                $scope.peopleResults.totalFound = results.totalCount;

                if (results.totalCount === 0) {
                    $scope.noResultsFound = true;
                }
                else {
                    $scope.isPeopleVisible = true;
                }
            }
            $scope.isLoading = false;
            $scope.$digest();
        }

        /**
         * Calls search factory to search games
         * @return {results} Array of game results
         * @method searchGames
         */
        function searchGames() {
            return SearchFactory.searchGames()
                .then(handleSearchGamesResults)
                .catch(function(error) {
                    ComponentsLogFactory.error('[Origin-Search-Results-Directive SearchGames Method] error', error);
                });
        }

        /**
         * Calls search factory to search store and people services
         * @return {results} Store and People Results Object
         * @method searchGames
         */
        function searchStoreAndPeople() {
            return SearchFactory.searchStoreAndPeople()
                .then(handleStoreAndPeopleResults)
                .catch(function(error) {
                    ComponentsLogFactory.error('[Origin-Search-Results-Directive searchStoreAndPeople Method] error', error);
                });
        }


        /**
         * Calls search factory to get more people service search results
         * @param  {page} search results page number
         * @return {void}
         * @method searchMorePeople
         */
        function searchMorePeople(start) {
            var searchKeyword = getSearchString();

            if (!AuthFactory.isClientOnline()) {
                return;
            }

            SearchFactory.searchPeople(searchKeyword, start)
            .then(handleMorePeopleResults)
            .catch(function(error) {
                ComponentsLogFactory.error('[Origin-Search-Results-Directive searchMorePeople Method] error', error);
            });
        }

        /**
         * Get more people search results.  Called when scrolled to bottom of results page.
         * @return {void}
         * @method loadMorePeopleResults
         */
        $scope.loadMorePeopleResults = function() {
            if (!$scope.isLoading && start < $scope.peopleResults.totalFound) {
                $scope.isLoading = true;
                searchMorePeople(start);
                $scope.$digest();
            }
        };

        function getSearchString() {
            return $stateParams.searchString ? decodeURI($stateParams.searchString) : '';
        }

        /**
         * Initialize variables and start a global search.
         * @param  {string} shouldDigest - should we digest after initializing vars?
                                           false if this is the initial search after creation of the directive;
                                           true if we are re-running the search after going on/offline, or logging in/out
         * @return {void}
         * @method beginSearch
         */
        function beginSearch(shouldDigest) {

            $scope.showingAllPeople  = false;
            $scope.isLoading = true;
            $scope.noResultsFound = false;
            $scope.isOffline = false;

            $scope.isGamesVisible = false;
            $scope.isStoreVisible = false;
            $scope.isPeopleVisible = false;
            $scope.peopleResultsLimit = 4;
            $scope.storeResultsLimit = 8;

            // If this is not the initial search, then digest now to reset the search results page before re-running the search.
            if (shouldDigest) {
                $scope.$digest();
            }

            if (AuthFactory.isClientOnline()) {
                // ONLINE
                if (AuthFactory.isAppLoggedIn()) {
                    // LOGGED IN
                    // Check the URL to see if category=people is included in the query string, for showing only people results
                    if ($stateParams.category === 'people') {
                        start = 0;

                        $scope.peopleResultsLimit = 'Infinity';
                        $scope.showingAllPeople = true;

                        // Initialize peopleResults since searchMorePeople() is expecting an initialized object
                        $scope.peopleResults = {
                            users: []
                        };

                        searchMorePeople(start);
                    }
                    else {
                        // Do a full search, with all category results
                        Promise.all([searchGames(), searchStoreAndPeople()])
                            .then(function(results){
                                $scope.isLoading = false;

                                // Send search results telemetry.
                                var gameLibraryResults = 0,
                                    storeResults = 0,
                                    peopleResults = 0;

                                if (results[0].length) {
                                    gameLibraryResults = results[0].length;
                                }
                                if (results[1].store) {
                                    storeResults = results[1].store.totalFound;
                                }
                                if (results[1].people) {
                                    peopleResults = results[1].people.totalFound;
                                }
                                Origin.telemetry.sendTelemetryEvent('TRACKER_DEV', 'search_result', 'user', 'search_result', {
                                    'searchString': getSearchString(),
                                    'gameLibraryResults': gameLibraryResults.toString(),
                                    'storeResults': storeResults.toString(),
                                    'peopleResults': peopleResults.toString()
                                });
                                Origin.telemetry.sendMarketingEvent('Global Search', 'Search', getSearchString());

                                if(!results[0].length && !results[1].store && !results[1].people){
                                     $scope.noResultsFound = true;
                                }
                                $scope.$digest();
                            });
                    }
                }
                else {
                    // logged out
                    $scope.storeResultsLimit = 18;

                    // When logged out, only do a store search.  The people portion of searchStoreAndPeople() will not return
                    // results in this case.
                    searchStoreAndPeople().then(function(results){
                        $scope.isLoading = false;

                        var storeResults = 0,
                            peopleResults = 0;

                        if (results.store) {
                            storeResults = results.store.totalFound;
                        }
                        if (results.people) {
                            peopleResults = results.people.totalFound;
                        }

                        Origin.telemetry.sendTelemetryEvent('TRACKER_DEV', 'search_result', 'user', 'search_result', {
                            'searchString': getSearchString(),
                            'gameLibraryResults': '0',
                            'storeResults': storeResults.toString(),
                            'peopleResults': peopleResults.toString()
                        });

                        if(storeResults === 0 && peopleResults === 0){
                             $scope.noResultsFound = true;
                        }
                        $scope.$digest();
                    });
                }
            }
            else {
                // OFFLINE - embedded browser, so only search games
                searchGames()
                    .then(function(results) {
                        $scope.isLoading = false;
                        $scope.isOffline = true;

                        if (!results.length) {
                             $scope.noResultsFound = true;
                        }
                        $scope.$digest();
                    });
            }
        }

        /**
         * Run another search, with digest.
         * @return {void}
         * @method rerunSearch
         */
        function rerunSearch() {
            beginSearch(true);
        }

        /**
         * Unhook from auth factory events when directive is destroyed.
         * @return {void}
         * @method onDestroy
         */
        function onDestroy() {
            EventsHelperFactory.detachAll(handles);
        }

        /**
         * When the auth factory has initialized, begin search.
         * @return {void}
         * @method onAuthReady
         */
        function onAuthReady() {
            beginSearch(false);
        }

        AuthFactory.waitForAuthReady()
            .then(onAuthReady)
            .catch(function(error) {
                ComponentsLogFactory.error('[Origin-Search-Results-Directive AuthReady] error', error);
            });

        handles = [
            AuthFactory.events.on('myauth:change', rerunSearch),
            AuthFactory.events.on('myauth:clientonlinestatechanged', rerunSearch)
        ];

        $scope.$on('$destroy', onDestroy);
    }

    /**
     * Global Search Resutls Page Directive
     */
    function originSearchPage(ComponentsConfigFactory) {

        /**
        * The directive link
        * Detect when the results page is scrolled down to the bottom, so that we can load more results, if necessary.
        */
        function originSearchPageLink(scope) {

            angular.element(window).off('scroll').on('scroll', function() {

                var top = (window.scrollY || window.pageYOffset);

                if ((window.innerHeight + top) >= document.body.scrollHeight && scope.showingAllPeople) {
                    scope.loadMorePeopleResults();
                }
            });
        }

        return {
            restrict: 'E',
            scope: {
                searchingStr: '@searching'
            },
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('search/views/search.html'),
            controller: 'OriginSearchPageCtrl',
            link: originSearchPageLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSearchPage
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} searching "Searching"
     * @description
     *
     * Global Search Main Page
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-search-page></origin-search-page>
     *     </file>
     * </example>
     */
    // declaration
    angular.module('origin-components')
        .controller('OriginSearchPageCtrl', OriginSearchPageCtrl)
        .directive('originSearchPage', originSearchPage);
}());