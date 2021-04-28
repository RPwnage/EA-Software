(function () {
    "use strict";

    function SearchFactory($stateParams, AuthFactory, GamesDataFactory, ObjectHelperFactory, ComponentsLogFactory) {

        var toArray = ObjectHelperFactory.toArray,
            getProperty = ObjectHelperFactory.getProperty,
            searchPromises = [],
            storeParams = {
                fq: 'gameType:basegame',
                sort: 'title asc',
                start: 0,
                rows: 20
            };

        /**
         * Creates a model object with game name and offerId
         * @param  {catalogData} array
         * @return {model} - game offerId and displayName
         */
        function createGamesModel(catalogData) {
            var model = {
                offerId: catalogData.offerId,
                displayName: catalogData.i18n.displayName
            };

            return model;
        }

        /**
         * Create an games Array with the data retrieved from getCatalogInfo call
         * @param  {catalogData}
         * @return {allGames} - array of owned games
         */
        function generateGamesModels(catalogData) {

            var allGames = toArray(catalogData).map(createGamesModel);
            return allGames;

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
                    ComponentsLogFactory.error('[SEARCHFACTORY] GamesDataFactory.getCatalogInfo', error.stack);
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
                searchText = decodeURI($stateParams.searchString),
                splitSearchText = searchText.toLowerCase().split(/\s+/),
                searchRegExp =  new RegExp("(?=.*" + splitSearchText.join(")(?=.*") + ")", "i");

            angular.forEach(games, function(game) {
                if(searchRegExp.test(game.displayName)){
                    searchGamesResults.push(game);
                }

            });

            return searchGamesResults;

        }

        /**
         * error triggered if retrieving the feed info is not successful
         * @param  {Error} error errorobject
         */
        function handleSearchGamesError(error) {
            ComponentsLogFactory.error('[SEARCH FACTORY - GAMES] Entitlement retrievel error', error.message);
        }



        /**
         * Wait for authReady then call People Search Service as it requires an authToken
         * @param  {searchString}
         * @param  {page} people results page number
         * @return {People Results} returns a promise 
         */
        function onAuthReadyPeople(searchString, page) {

            return function (loginObj) {

                if (loginObj && loginObj.isLoggedIn) {
                    return Origin.search.searchPeople(searchString, page);
                }
                else {
                    return Promise.resolve({});
                }

            };

        }

        /**
         * In case there is an auth error, resolve the promise and return an empty object
         * @param  {error}
         * @return {} - resolves the promise
         */
        function onAuthReadyError(error) {

            ComponentsLogFactory.error('SEARCHFACTORY - waitForAuthReady error:', error.message);
            return Promise.resolve({});

        }

        /**
         * Generates a resultset and returns to the directive .
         * @param  {searchResults} Results returned from store and people service
         * @return {resultSet} creates and object with results from store and people service
         */
        function handleSearchResults(searchResults) {

            var resultSet = {};

            if(searchResults[0] && searchResults[0].result.games.game) {
                resultSet.store = {
                    games : searchResults[0].result.games.game,
                    totalFound : searchResults[0].result.games.numFound
                };
            }

            if(searchResults[1] && searchResults[1].infoList && searchResults[1].infoList.length > 0){
                resultSet.people = {
                    users : searchResults[1].infoList,
                    totalFound : searchResults[1].totalCount
                };
            }

            return resultSet;
        }

        /**
         * Search Games - Check for authready --> check for games ready --> get Owned games list --> 
         * filter out games based on the search term using OR logic 
         * @return {searchGamesResults} - after the chain of promise resolves, an array of games which matched the searchString
         * will be returned
         */
        function searchGames() {
            return AuthFactory.waitForAuthReady()
                .then(waitForGamesReady)
                .then(getOwnedOfferIds)
                .then(getGamesResults)
                .catch(handleSearchGamesError);
        }

        /**
         * generate results by calling store search service based on the search string
         * @param  {searchString} search string
         * @param  {storeParams} params for store service
         * @return {promise} resolves after response recieved from store proxy service
         * NOTE: This is needed by store scrum to generate the carousels data
         */
        function searchStore(searchString, storeParams) {

            return Origin.search.searchStore(searchString, storeParams).catch(function(error) {
                ComponentsLogFactory.error('SEARCHFACTORY - store service error:', error.message);
            });

        }

        /**
         * generate results by calling people search service based on the search string
         * @param  {page} people results page number
         * @return {promise} resolves after response received from people service
         */
        function searchPeople(page) {

            var searchString = decodeURI($stateParams.searchString);

            return AuthFactory.waitForAuthReady()
                .then(onAuthReadyPeople(searchString, page))
                .catch(onAuthReadyError);

        }

        /**
         * generates an array of search results
         * @return {promise} resolves after getting the results from store and people service
         */
        function searchStoreAndPeople() {

            var searchString = decodeURI($stateParams.searchString);
            searchPromises = [
                searchStore(searchString, storeParams),
                searchPeople(1)
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

    function SearchFactorySingleton($stateParams, AuthFactory, GamesDataFactory, ObjectHelperFactory, ComponentsLogFactory, SingletonRegistryFactory) {
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