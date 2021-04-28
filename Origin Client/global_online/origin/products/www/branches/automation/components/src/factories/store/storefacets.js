/**
 * @file store/storesearch.js
 */
(function () {
    'use strict';

    var NORMAL_FILTER = false;
    var DEFAULT_FILTER_MODEL = {filterType: NORMAL_FILTER, value: false};

    function StoreFacetFactory(ResultFilterFactory) {
        var sortBy = null,
            facets = {};

        /**
         * Register and create a facet category option
         *
         * @param {string} categoryName - category name
         * @param {string} optionName - option name
         */
        function registerSortOption(optionName){
            if (!sortBy){
                sortBy =  ResultFilterFactory.getInstance();
            }
            sortBy.addFilter(DEFAULT_FILTER_MODEL, optionName);
        }

        /**
         * Register and create a facet category option
         *
         * @param {string} categoryName - category name
         * @param {string} optionName - option name
         */
        function registerFacetOption(categoryName, optionName){
            if (!facets[categoryName]){
                facets[categoryName] = ResultFilterFactory.getInstance();
            }
            facets[categoryName].addFilter(DEFAULT_FILTER_MODEL, optionName);
        }

        function getFacets() {
            return facets;
        }

        function getSortOrders() {
            return sortBy;
        }

        return {
            getFacets: getFacets,
            getSortOrders: getSortOrders,
            registerFacetOption: registerFacetOption,
            registerSortOption: registerSortOption
        };
    }

    angular.module('origin-components')
        .factory('StoreFacetFactory', StoreFacetFactory);

}());
