/**
 * @file store/storesearchquery.js
 * @description Handles query parameter changes/ retrieval for store search.
 */
(function() {
    'use strict';

    function StoreSearchQueryFactory( $stateParams, StoreFacetFactory) {

        var facets = StoreFacetFactory.facets,
            sortOrders = StoreFacetFactory.sortOrders;

        function isNotEmpty(string) {
            return string ? true: false;
        }

        function getFacetCategories() {
            return Object.keys(facets);
        }

        function getQueryParamByKey(key) {
            if (!$stateParams[key]) {
                return '';
            }

            return decodeURI($stateParams[key]);
        }

        /**
         * Evaluate querystring's "searchString" value and return it if present, else empty string
         *
         * @return {string}
         */
        function getSearchStringQueryParam() {
            return getQueryParamByKey('searchString');
        }

        /**
         * Evaluate querystring's "fq" value and return it if present, else empty string
         *
         * @return {string}
         */
        function getFacetsQueryParam() {
            return getQueryParamByKey('fq');
        }

        /**
         * Evaluate querystring's "sort" value and return it if present, else empty string
         *
         * @return {string}
         */
        function getSortQueryParam() {
            return getQueryParamByKey('sort');
        }

        /**
         * Returns currently active facet value
         * @param values
         * @param facetKey
         * @returns {*}
         */
        function getActiveFacets(values, facetKey) {
            var activeValues = Object.keys(values.getApplicableFilterValues());
            if (activeValues.length > 0) {
                // For now just take head. We are not sure whether we support multiple values or not
                return (facetKey ? (facetKey + ':') : '') + activeValues[0];
            } else {
                return '';
            }
        }

        /**
         * Constructs search service parameter for active facets
         *
         * @return {string}
         */
        function getActiveFacetsAsString() {
            return Object.keys(facets).map(function (facetKey) {
                return getActiveFacets(facets[facetKey], facetKey);
            }).filter(isNotEmpty).join(',');
        }


        function getSortAsString() {
            return getActiveFacets(sortOrders);
        }

        function extractFacetValuesFromQuery(queryParams) {
            var keyValuePairs = {};
            if (queryParams) {
                queryParams.split(',').map(function (facetGroupWithValues) {
                    var keyValuePair = facetGroupWithValues.split(':');
                    if (keyValuePair.length > 1 && facets[keyValuePair[0]]) {
                        keyValuePairs[keyValuePair[0]] = keyValuePair[1];
                    }
                });
            }
            return keyValuePairs;
        }

        function updateFacetValuesFromQuery(queryParam) {
            var keyValuePairs = extractFacetValuesFromQuery(queryParam);
            for (var key in keyValuePairs) {
                facets[key].setFilterValue(keyValuePairs[key], true);
            }
        }

        function updateSortValuesFromQuery(sortOrderValue) {
            if (sortOrderValue) {
                sortOrders.setFilterValue(sortOrderValue, true);
            }
        }

        return {
            getFacetCategories: getFacetCategories,
            getFacetsQueryParam: getFacetsQueryParam,
            getSearchStringQueryParam: getSearchStringQueryParam,
            getSortQueryParam: getSortQueryParam,
            getSortAsString: getSortAsString,
            getActiveFacetsAsString: getActiveFacetsAsString,
            updateFacetValuesFromQuery: updateFacetValuesFromQuery,
            updateSortValuesFromQuery: updateSortValuesFromQuery
        };
    }

    angular.module('origin-components')
        .factory('StoreSearchQueryFactory', StoreSearchQueryFactory);
}());