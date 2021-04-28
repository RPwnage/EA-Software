(function () {
    'use strict';

    var PEOPLE_RESULTS_PER_PAGE = 20;

    /**
     * Global Search Resutls Page Ctrl
     */
    function OriginSearchPageCtrl($scope, $state, $stateParams, SearchFactory, ComponentsLogFactory, AuthFactory) {

        var morePeoplePageNumber;


        /**
         * Shows all of the people search results and hides other search category results
         * @return {void}
         * @method showAllPeopleCallback
         */
        $scope.showAllPeopleCallback = function() {

            $scope.isGamesVisible = false;
            $scope.isStoreVisible = false;
            $scope.peopleResultsLimit = 'Infinity';
            $scope.showingAllPeople  = true;

            window.scrollTo(0, 0);

            $state.go($state.current, {
                searchString: decodeURI($stateParams.searchString),
                category: 'people'
            });
        };

        /**
         * Get more people search results.  Called when scrolled to bottom of results page.
         * @return {void}
         * @method loadMorePeopleResults
         */
        $scope.loadMorePeopleResults = function() {

            var totalResultsPages = Math.ceil($scope.peopleResults.totalFound * 1.0 / PEOPLE_RESULTS_PER_PAGE);

            if (!$scope.isLoading && morePeoplePageNumber <= totalResultsPages) {
                $scope.isLoading = true;
                searchMorePeople(morePeoplePageNumber++);
            }
        };

        /**
         * When the auth factory has initialized, hook into its events, and begin search.
         * @return {void}
         * @method onAuthReady
         */
        function onAuthReady() {
            // When the user logs in/out of SPA, or client goes on/offline, 
            // re-run the search with digest after initialization, to reset the results page.
            AuthFactory.events.on('myauth:change', rerunSearch);
            AuthFactory.events.on('myauth:clientonlinestatechanged', rerunSearch);
            $scope.$on('$destroy', onDestroy);

            beginSearch(false);
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
         * @return {void}
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

            $scope.peopleResults.users.push.apply($scope.peopleResults.users, results.infoList);
            $scope.peopleResults.totalFound = results.totalCount;

            if (results.totalCount === 0) {
                $scope.noResultsFound = true;
            }
            else {
                $scope.isPeopleVisible = true;
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
                    ComponentsLogFactory.error('[Origin-Search-Results-Directive SearchGames Method] error', error.message);
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
                    ComponentsLogFactory.error('[Origin-Search-Results-Directive searchStoreAndPeople Method] error', error.message);
                });
        }

        /**
         * Calls search factory to get more people service search results
         * @param  {page} search results page number        
         * @return {void}
         * @method searchMorePeople
         */
        function searchMorePeople(page) {

            if (!AuthFactory.isClientOnline()) {
                return;
            }

            SearchFactory.searchPeople(page)
            .then(handleMorePeopleResults)
            .catch(function(error) {
                ComponentsLogFactory.error('[Origin-Search-Results-Directive searchMorePeople Method] error', error.message);
            });
        }

        /**
         * Unhook from auth factory events when directive is destroyed.
         * @return {void}
         * @method onDestroy
         */
        function onDestroy() {
            AuthFactory.events.off('myauth:change', rerunSearch);
            AuthFactory.events.off('myauth:clientonlinestatechanged', rerunSearch);
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
            morePeoplePageNumber = 2;

            $scope.showingAllPeople  = false;
            $scope.isLoading = true;
            $scope.noResultsFound = false;
            $scope.loggedOut = false;
            $scope.isOffline = false;

            $scope.isGamesVisible = false;
            $scope.isStoreVisible = false;
            $scope.isPeopleVisible = false;
            $scope.peopleResultsLimit = 4;

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

                        $scope.peopleResultsLimit = 'Infinity';
                        $scope.showingAllPeople = true;

                        // Initialize peopleResults since searchMorePeople() is expecting an initialized object
                        $scope.peopleResults = {
                            users: []
                        };

                        searchMorePeople(1);
                    }
                    else {
                        // Do a full search, with all category results
                        Promise.all([searchGames(), searchStoreAndPeople()])
                            .then(function(results){
                                $scope.isLoading = false;

                                if(!results[0].length && results[1].store === undefined && results[1].people === undefined){
                                     $scope.noResultsFound = true;
                                }
                                $scope.$digest();
                            });
                    }
                }
                else {
                    // LOGGED OUT
                    $scope.loggedOut = true;

                    // When logged out, only do a store search.  The people portion of searchStoreAndPeople() will not return
                    // results in this case.
                    searchStoreAndPeople()
                        .then(function(results){
                            $scope.isLoading = false;

                            if(results.store === undefined && results.people === undefined){
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


        AuthFactory.waitForAuthReady().then(onAuthReady)
            .catch(function(error) {
                ComponentsLogFactory.error('[Origin-Search-Results-Directive AuthReady] error', error.message);
            });
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

                if ((window.innerHeight + window.scrollY) >= document.body.offsetHeight && scope.showingAllPeople) {
                    scope.loadMorePeopleResults();
                }
            });
        }

        return {
            restrict: 'E',
            scope: true,
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