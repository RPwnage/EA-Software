/**
 * @file common/resultfilters.js
 */

(function() {
    'use strict';

    function ResultFilterFactory(ObjectHelperFactory) {

        /**
         * Given a set of filters, it provides utility functions to operate on those filters
         * @param {Object} filters
         * @returns {{getFilters: getFilters, getFilter: getFilter, isActive: isActive, getAllFilterValues: getAllFilterValues, setAllFilterValues: setAllFilterValues, getApplicableFilterValues: getApplicableFilterValues, isFilterActive: isFilterActive, countActiveFilters: countActiveFilters, dismissFilter: dismissFilter, dismissAll: dismissAll, deactivate: deactivate}}
         */
        function getInstance(filters) {
            // Shortcuts
            var map = ObjectHelperFactory.map,
                filter = ObjectHelperFactory.filter,
                getProperty = ObjectHelperFactory.getProperty,
                length = ObjectHelperFactory.length;

            /**
             * Creates filter model object.
             * @param {mixed} filterType -  normal filters apply when active, inverted - when inactive.
             *    For example:
             *      -- isFavorite is an normal filter, meaning that only favorite games will be in the list
             *                    when this filter is active and all the games otherwise
             *      -- isHidden is an inverted filter, meaning that all the games will be in the list
             *                    when this filter is active and only non-hidden games otherwise
             * @param {mixed} value - *optional* initial filter value
             * @return {object}
             */
            function createFilterObject(filter) {
                return {
                    value: filter.value,
                    count: 0,
                    inverted: filter.filterType
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
            function deactivate(filterObject) {
                filterObject.value = (typeof filterObject.value === 'string') ? '' : false;
            }

            /**
             * Checks if the given filter should be applied or not based on its value and configuration.
             * @param {object} filterObject - filter model object
             * @return {boolean}
             */
            function isApplicable(filterObject) {
                return (isActive(filterObject) && !filterObject.inverted) || (!isActive(filterObject) && filterObject.inverted);
            }

            /**
             * Returns all filters
             * @return {object}
             */
            function getFilters() {
                return filters;
            }

            /**
             * Returns filter model by its name.
             * @param {string} filterName - filter names correspond to game model property names
             * @return {object}
             */
            function getFilter(filterName) {
                return filters[filterName];
            }

            function setFilterAttribute(filterName, attr, value) {
                var filterObject = getFilter(filterName);
                if (filterObject) {
                    filterObject[attr] = (typeof value === 'object') ? value[attr] : value;
                }
            }

            /**
             * Sets filter value
             * @param {string} filterName - filter names correspond to game model property names
             * @param {mixed} value - new value or another filter object that we want to take the new value from
             * @return {void}
             */
            function setFilterValue(filterName, value) {
                setFilterAttribute(filterName, 'value', value);
            }

            function setFilterCount(filterName, count) {
                setFilterAttribute(filterName, 'count', count);
            }


            /**
             * Checks filter status by its name.
             * @param {string} filterName - filter names correspond to game model property names
             * @return {boolean}
             */
            function isFilterActive(filterName) {
                var filterObject = getFilter(filterName);
                if (filterObject) {
                    return isActive(filterObject);
                }
                return false;
            }

            /**
             * Inactivates filter by its name
             * @param {string} filterName - filter names correspond to game model property names
             * @return {void}
             */
            function dismissFilter(filterName) {
                var filterObject = getFilter(filterName);
                if (filterObject) {
                    deactivate(filterName);
                }
            }

            /**
             * Sets filter values in bulk
             * @param {object} values - new values indexed by filter name
             * @return {void}
             */
            function setAllFilterValues(values) {
                if (length(values)) {
                    angular.forEach(values, function (value, name) {
                        setFilterValue(name, value);
                    });
                }
            }

            function setAllFilterCounts(values) {
                if (length(values)) {
                    angular.forEach(values, function (value) {
                        setFilterCount(value.name, value);
                    });
                }
            }

            /**
             * Returns all filter values
             * @return {object}
             */
            function getAllFilterValues() {
                return map(getProperty('value'), filters);
            }

            /**
             * Inactivates all filters.
             * @return {void}
             */
            function dismissAll() {
                map(deactivate, filters);
            }

            /**
             * Returns the number of currently active filters.
             * @return {integer}
             */
            function countActiveFilters() {
                return length(filter(isActive, filters));
            }

            /**
             * Returns values for all the filters that can be applied as a Javascript object.
             * @return {object}
             */
            function getApplicableFilterValues() {
                return map(getProperty('value'), filter(isApplicable, filters));
            }

            if (filters) {
                filters = map(createFilterObject, filters);
            }

            return {
                getFilters: getFilters,
                getFilter: getFilter,
                isActive: isActive,
                setFilterValue: setFilterValue,
                setFilterCount: setFilterCount,
                getAllFilterValues: getAllFilterValues,
                setAllFilterValues: setAllFilterValues,
                setAllFilterCounts: setAllFilterCounts,
                getApplicableFilterValues: getApplicableFilterValues,
                isFilterActive: isFilterActive,
                countActiveFilters: countActiveFilters,
                dismissFilter: dismissFilter,
                dismissAll: dismissAll,
                deactivate: deactivate
            };
        }

        return {
            getInstance: getInstance
        };
    }

    angular.module('origin-components')
        .factory('ResultFilterFactory', ResultFilterFactory);
}());
