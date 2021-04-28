(function () {
    "use strict";

    function SearchFactory($stateParams, AuthFactory, GamesDataFactory, ObjectHelperFactory, ComponentsLogFactory, GamesFilterFactory) {

        var toArray = ObjectHelperFactory.toArray,
            getProperty = ObjectHelperFactory.getProperty,
            searchPromises = [],
            allGames = [],
            storeParams = {
                sort: 'rank desc,releaseDate desc,title desc',
                start: 0,
                rows: 20
            };

        /**
         * Update Game Filters and creates an array for filtered games
         * @return {filteredGames} - array of owned games
         */
        function updateGameFilters() {
            allGames.forEach(GamesFilterFactory.updateGameModelExtraProperties);
            GamesFilterFactory.hideLesserEditions(allGames);
            var filteredGames = GamesFilterFactory.applyFilters(allGames);
            return filteredGames;
        }

        /**
         * Create an games Array with the data retrieved from getCatalogInfo call
         * @param  {catalogData}
         * @return {allGames} - array of owned games
         */
        function generateGamesModels(catalogData) {
            allGames = toArray(catalogData).map(GamesFilterFactory.createGameModel);
        }

        /**
         * Call the baseGameEntitlements to get offerids, call getCatalogInfo to retrieve displayName
         * and then create an object of owned games(game - displayName and offerId)
         * @param  {}
         * @return {games} array of owned games
         */
        function getOwnedOfferIds() {
            var offerids = GamesDataFactory.baseGameEntitlements().map(getProperty('offerId'));
            return GamesDataFactory.getCatalogInfo(offerids)
                .then(generateGamesModels)
                .catch(function(error) {
                    ComponentsLogFactory.error('[SEARCHFACTORY] GamesDataFactory.getCatalogInfo', error);
                });
        }

        /**
         * Call the baseGameEntitlements fn after the parent fn has initalized
         * Resolves the promise once the baseGameEntitlementUpdate event is fired
         */
        function waitForGamesReady() {
            return new Promise(function(resolve) {
              if (GamesDataFactory.initialRefreshCompleted()) {
                resolve();
              } else {
                //setup a listener for any entitlement updates
                GamesDataFactory.events.once('games:baseGameEntitlementsUpdate', resolve);
              }
            });
        }

        /**
         * Search for game names based on the searchString.
         * @param  {games} array of owned games
         * @return {searchGamesResults} array of games which matched the search string
         */
        function getGamesResults(games) {
            var searchGamesResults = [],
                searchText = decodeURI($stateParams.searchString).toLowerCase(),
                splitSearchText = searchText.toLowerCase().split(/\s+/);

            for(var i=0; i < games.length; i++) {
                var count = 0;
                for(var j=0; j < splitSearchText.length; j++) {
                    if(games[i].displayName !== null && games[i].displayName.toLowerCase().indexOf(splitSearchText[j]) !== -1) {
                        count++;
                    }
                }
                if(count === splitSearchText.length) {
                    searchGamesResults.push(games[i]);
                }
            }

            return searchGamesResults;
        }

        /**
         * error triggered if retrieving the feed info is not successful
         * @param  {Error} error errorobject
         */
        function handleSearchGamesError(error) {
            ComponentsLogFactory.error('[SEARCH FACTORY - GAMES] Entitlement retrievel error', error);
            Origin.telemetry.sendTelemetryEvent('TRACKER_DEV', 'search_error', 'user', 'search_error', { 'error': error.message });
        }



        /**
         * Wait for authReady then call People Search Service as it requires an authToken
         * @param  {string} searchString
         * @param  {Number} start results row number from where to start
         * @return {Function} returns a function that once invoked returns a promise with list of userIds
         */
        function onAuthReadyPeople(searchString, start) {

            return function (loginObj) {

                if (loginObj && loginObj.isLoggedIn) {
                    return Origin.search.searchPeople(searchString, start);
                }
                else {
                    return Promise.resolve({});
                }

            };

        }

        /**
         * In case there is an auth error, resolve the promise and return an empty object
         * @param  {string} error
         * @return {Promise} - resolves the promise
         */
        function onAuthReadyError(error) {

            ComponentsLogFactory.error('SEARCHFACTORY - waitForAuthReady error:', error);
            return Promise.resolve({});

        }

        /**
         * Generates a resultset and returns to the directive .
         * @param  {Array} searchResults returned from store and people service
         * @return {Object} creates an object with results from store and people service
         */
        function handleSearchResults(searchResults) {

            var resultSet = {};

            resultSet.store = handleStoreSearchResults(searchResults[0]);
            resultSet.people = handlePeopleSearchResults(searchResults[1]);

            return resultSet;
        }


        function handleStoreSearchResults(storeSearchResults) {
            var games = _.get(storeSearchResults, ['games', 'game'], []);
            var totalCount = _.get(storeSearchResults, ['games', 'numFound'], 0);
            if(totalCount > 0) {
                return {
                    games : games,
                    totalFound : totalCount
                };
            }

        }

        function handlePeopleSearchResults(peopleSearchResults) {
            var infoList = _.get(peopleSearchResults, 'infoList', []);
            var totalCount = _.get(peopleSearchResults, 'totalCount', 0);
            if (totalCount > 0) {
                return {
                    users: infoList,
                    totalFound: totalCount
                };
            }
        }

        /**
         * Search Games - Check for authready --> check for games ready --> get Owned games list --> 
         * filter out games based on the search term using OR logic 
         * @return {Promise} - after the chain of promise resolves, an array of games which matched the searchString
         * will be returned
         */
        function searchGames() {
            return AuthFactory.waitForAuthReady()
                .then(waitForGamesReady)
                .then(getOwnedOfferIds)
                .then(GamesDataFactory.waitForFiltersReady)
                .then(updateGameFilters)
                .then(getGamesResults)
                .catch(handleSearchGamesError);
        }

        /**
         * generate results by calling store search service based on the search string
         * @param  {string} searchString string
         * @param  {Object} storeParams for store service
         * @return {promise} resolves after response recieved from store proxy service
         * NOTE: This is needed by store scrum to generate the carousels data
         */
        function searchStore(searchString, storeParams) {

            return Origin.search.searchStore(searchString, storeParams).catch(function(error) {
                ComponentsLogFactory.error('SEARCHFACTORY - store service error:', error);
                Origin.telemetry.sendTelemetryEvent('TRACKER_DEV', 'search_error', 'user', 'search_error', { 'error': error.message });
                return Promise.reject(error);
            });

        }

        /**
         * generate results by calling people search service based on the search string
         * @param {string} searchKeyword
         * @param  {Number} start results row number from where to start
         * @return {promise} resolves after response received from people service
         */
        function searchPeople(searchKeyword, start) {
            var searchString = searchKeyword || decodeURI($stateParams.searchString);

            return AuthFactory.waitForAuthReady()
                .then(onAuthReadyPeople(searchString, start))
                .catch(onAuthReadyError);

        }

        /**
         * generates an array of search results
         * @return {promise} resolves after getting the results from store and people service
         */
        function searchStoreAndPeople() {
            var start = 0;
            var searchString = decodeURI($stateParams.searchString);
            searchPromises = [
                searchStore(searchString, storeParams),
                searchPeople(searchString, start)
            ];

            return Promise.all(searchPromises).then(handleSearchResults);

        }

        return {

            /**
             * generate results by calling store search service based on the search string 
             * @param  {searchString, StoreParams} search string and params for store service
             * @return {promise} resolves after response recieved from store proxy service
             */
            searchStore : searchStore,

            /**
             * generate results by calling GamesDataFactory based on the search string 
             * @return {promise} resolves after response recieved from store proxy service
             */
            searchGames : searchGames,

            /**
             * generates an array of search results
             * @return {promise} resolves after getting the results from store and people service
             */
            searchStoreAndPeople : searchStoreAndPeople,

            /**
             * generates results by calling people search service based on the search string
             * @return {promise} resolves after getting the results from people service
             */
            searchPeople : searchPeople
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */

    function SearchFactorySingleton($stateParams, AuthFactory, GamesDataFactory, ObjectHelperFactory, ComponentsLogFactory, GamesFilterFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('SearchFactory', SearchFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.SearchFactory

     * @description
     *
     * SearchFactory
     */
    angular.module('origin-components')
        .factory('SearchFactory', SearchFactorySingleton);
}());