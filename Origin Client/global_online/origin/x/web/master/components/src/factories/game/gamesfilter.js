/**
 * @file game/gamefilters.js
 */

(function() {
    'use strict';

    var STORAGE_KEY = 'origin-game-library-filters:',
        NORMAL_FILTER = false,
        INVERTED_FILTER = true;

    function GamesFilterFactory($filter, ObjectHelperFactory, LocalStorageFactory, ResultFilterFactory) {
        // Shortcuts
        var filterService = $filter('filter'),
            length = ObjectHelperFactory.length,
            forEach = ObjectHelperFactory.forEach,

            // Initialize filters
            gameFilterValues = LocalStorageFactory.get(STORAGE_KEY, {}),
            gameFilters = {
                displayName: {filterType: NORMAL_FILTER, value: ''},
                isPlayedLastWeek: {filterType: NORMAL_FILTER, value: false},
                isFavorite: {filterType: NORMAL_FILTER, value: false},
                isDownloading: {filterType: NORMAL_FILTER, value: false},
                isHidden: {filterType: INVERTED_FILTER, value: false},
                isInstalled: {filterType: NORMAL_FILTER, value: false},
                isNonOrigin: {filterType: NORMAL_FILTER, value: false},
                isSubscription: {filterType: NORMAL_FILTER, value: false},
                isPcGame: {filterType: NORMAL_FILTER, value: false},
                isMacGame: {filterType: NORMAL_FILTER, value: false}
            },
            resultFilterFactoryInstance = ResultFilterFactory.getInstance(gameFilters);

        resultFilterFactoryInstance.setAllFilterValues(gameFilterValues);

        /**
         * Applies active filters to the given collection of models.
         * @param {object} collection - collection to filter
         * @return {object}
         */
        function applyFilters(collection) {
            var filterValues = resultFilterFactoryInstance.getApplicableFilterValues();

            LocalStorageFactory.set(STORAGE_KEY, resultFilterFactoryInstance.getAllFilterValues());

            if (length(filterValues) > 0) {
                collection = filterService(collection, filterValues);
            }

            return collection;
        }

        /**
         * Counts matching objects for a filter.
         * @param {string} filterName - filter names correspond to game model property names
         * @param {object} collection - collection to filter
         * @return {integer}
         */
        function getCountForFilter(filterName, collection) {
            var filterObject = {};

            filterObject[filterName] = true;

            return filterService(collection, filterObject).length;
        }

        /**
         * Counts matching objects for all the filters: active or inactive.
         * @param {object} collection - collection to filter
         * @return {object}
         */
        function calculateCounts(collection) {
            forEach(function (filterObject, filterName) {
                filterObject.count = getCountForFilter(filterName, collection);

                // Active filter that's lost all matching items should be de-activated
                if (resultFilterFactoryInstance.isActive(filterObject) && filterObject.count === 0) {
                    resultFilterFactoryInstance.deactivate(filterObject);
                }
            }, resultFilterFactoryInstance.getFilters());

            return collection;
        }

        return angular.extend({
            applyFilters: applyFilters,
            getCountForFilter: getCountForFilter,
            calculateCounts: calculateCounts
        }, resultFilterFactoryInstance);

    }

    angular.module('origin-components')
        .factory('GamesFilterFactory', GamesFilterFactory);
}());
