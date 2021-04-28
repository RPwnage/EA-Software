/**
 * @file gamelibrary/loggedin.js
 */

(function() {
    'use strict';

    var STORAGE_KEY = 'origin-game-library-filters:';

    function GamesFilterFactory($filter, ObjectHelperFactory, ComponentsLogFactory) {
        // Shortcuts
        var filterService = $filter('filter'),
            map = ObjectHelperFactory.map,
            filter = ObjectHelperFactory.filter,
            getProperty = ObjectHelperFactory.getProperty,
            length = ObjectHelperFactory.length;

        // State
        var filters = {};

        // Configuration
        var defaultFilterValues = {
            displayName: '',
            isPlayedLastWeek: false,
            isFavorite: false,
            isDownloading: false,
            isHidden: false,
            isInstalled: false,
            isNonOrigin: false,
            isSubscription: false,
            isPcGame: false,
            isMacGame: false
        };

        /**
         * Generates local storage key for the current user.
         * @return {string}
         */
        function getStorageKey() {
            return STORAGE_KEY + Origin.user.userPid();
        }

        /**
         * Creates filter model object.
         * @param {mixed} value - *optional* initial filter value
         * @return {object}
         */
        function createFilterObject(value) {
            return {
                value: value,
                itemsCount: 0
            };
        }

        /**
         * Checks if the given filter is active or not.
         * @param {object} filterObject - filter model object
         * @return {boolean}
         */
        function isActive(filterObject) {
            var filterValue = filterObject.value;

            return (typeof filterValue === 'string' && filterValue.length > 0) || !!filterValue;
        }

        /**
         * Inactivates given filter
         * @param {object} filterObject - filter model object
         * @return {void}
         */
        function inactivate(filterObject) {
            filterObject.value = (typeof filterObject.value === 'string') ? '' : false;
        }

        /**
         * Returns all active filter values in a Javascript object.
         * @return {object}
         */
        function getActiveFilterValues() {
            return map(getProperty('value'), filter(isActive, filters));
        }

        /**
         * Returns filter model by its name.
         * @param {string} filterName - filter names correspond to game model property names
         * @return {object}
         */
        function getFilter(filterName) {
            if (!filters.hasOwnProperty(filterName)) {
                filters[filterName] = createFilterObject();
            }

            return filters[filterName];
        }

        /**
         * Checks filter status by its name.
         * @param {string} filterName - filter names correspond to game model property names
         * @return {boolean}
         */
        function isFilterActive(filterName) {
            return isActive(getFilter(filterName));
        }

        /**
         * Inactivates filter by its name
         * @param {string} filterName - filter names correspond to game model property names
         * @return {void}
         */
        function dismissFilter(filterName) {
            inactivate(getFilter(filterName));
        }

        /**
         * Inactivates all filters.
         * @return {void}
         */
        function dismissAll() {
            map(inactivate, filters);
        }

        /**
         * Returns the number of currently active filters.
         * @return {integer}
         */
        function countActiveFilters() {
            return length(getActiveFilterValues());
        }

        /**
         * Applies active filters to the given collection of models.
         * @param {object} collection - collection to filter
         * @return {object}
         */
        function applyActiveFilters(collection) {
            var activeFilterValues = getActiveFilterValues();

            saveFilters(filters);

            if (length(activeFilterValues) > 0) {
                collection = filterService(collection, activeFilterValues);
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
            angular.forEach(filters, function (filterObject, filterName) {
                filterObject.itemsCount = getCountForFilter(filterName, collection);
            });

            return collection;
        }

        /**
         * Saves filters for the current user.
         * @param {object} data - state to save
         * @return {void}
         */
        function saveFilters(data) {
            var key = getStorageKey();
            localStorage[key] = JSON.stringify(data);
        }

        /**
         * Restores filters from previous session.
         * @return {object}
         */
        function readLastSavedFilters() {
            var key = getStorageKey(), savedFilters = {};

            if (typeof localStorage[key] !== "undefined") {
                try {
                    savedFilters = JSON.parse(localStorage[key]);
                } catch (error) {
                    // We don't really care about read errors: just use default filters in this case
                    ComponentsLogFactory.error('[games-filter-factory readLastSavedFilters] error', error.message);
                }
            }

            return savedFilters;
        }

        // Initialize filters
        filters = readLastSavedFilters();
        if (!filters || !length(filters)) {
            filters = map(createFilterObject, defaultFilterValues);
        }

        return {
            getFilter: getFilter,
            isFilterActive: isFilterActive,
            countActiveFilters: countActiveFilters,
            applyActiveFilters: applyActiveFilters,
            dismissFilter: dismissFilter,
            dismissAll: dismissAll,
            calculateCounts: calculateCounts
        };
    }

    angular.module('origin-components')
        .factory('GamesFilterFactory', GamesFilterFactory);
}());
