/**
 * @file store/storesearchquery.js
 * @description Handles query parameter changes/ retrieval for store search.
 */
(function() {
    'use strict';

    function StoreSearchQueryFactory( $stateParams, $location, StoreFacetFactory, UrlHelper) {

        var facets = StoreFacetFactory.getFacets,
            sortOrders = StoreFacetFactory.getSortOrders;

        function isNotEmpty(string) {
            return string ? true: false;
        }

        function getFacetCategories() {
            return Object.keys(facets());
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
            var activeValues = Object.keys(values.getApplicableFilterValues()).sort();
            if (activeValues.length > 0) {
                return _.map(activeValues, function(value) {
                    return (facetKey ? (facetKey + ':') : '') + value;
                });
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
            var facetsKeyValues = facets();
            return Object.keys(facetsKeyValues).map(function (facetKey) {
                return getActiveFacets(facetsKeyValues[facetKey], facetKey);
            }).filter(isNotEmpty).sort().join(',');
        }


        function getSortAsString() {
            return getActiveFacets(sortOrders(), false);
        }

        /**
         * Splits a comma delimited string of search facet key-value pairs. It  takes into consideration 
         * some values may have comma(s) or colons in them for example: 'developer:volition,-inc' and 
         * should not split into 2 separate queries in that case.
         * @param  {string} queryParams facets query string
         * @return {array}
         */
        function splitCommaDelimitedFacetQueryParam(queryParam) {
            queryParam = queryParam || '';
            return _.reduce(queryParam.split(','), function(queries, query) {
                var keyValuePair = query.split(':');
                if(queries.length && keyValuePair.length === 1){
                    queries[queries.length - 1] += ',' + query;
                } else if(query) {
                    queries.push(query);
                } 
                return queries;
            }, []);
        }

        /**
         * Parses queryString facets and returns array of active pairs
         * @param  {string} facetQueryStringParam facets query string
         * @return {array}
         */
        function extractFacetValuesFromQuery(facetQueryStringParam) {
            var facetsKeyValues = facets();
            var queryParams = splitCommaDelimitedFacetQueryParam(facetQueryStringParam);
            return _.reduce(queryParams, function(queries, keyValuePair) {
                var facetCategory, facetValue;
                keyValuePair = _.slice(keyValuePair.split(/:(.+)/), 0, 2); // split at first occurence of :
                facetCategory = keyValuePair[0];
                facetValue = keyValuePair[1];
                if(facetsKeyValues[facetCategory]) {
                    queries.push(keyValuePair);
                }
                return queries;
            }, []);
        }


        /**
         * Set facets as active based on key/value pair arrays provided
         * @param  {string} queryParam array of pair arrays e.g. 'franchise:command-and-conquer,genre:strategy'
         */
        function updateFacetValuesFromQuery(queryParam) {
            var keyValuePairs = extractFacetValuesFromQuery(queryParam);
            var facetsKeyValues = facets();
            _.forEach(keyValuePairs, function(keyValuePair) {
                facetsKeyValues[keyValuePair[0]].setFilterValue(keyValuePair[1], true);
            });
        }

        /**
         * Construct search string from a query parameter object.
         * @param parameters {
         *                      'fq': 'genre:action,genre:adventure',
         *                      'sort': 'dynamicPrice acs'
         *                   }
         * @returns {string} search string
         */
        function getSearchStringFromParameters(parameters){
            var searchString;
            _.forEach(parameters, function(value, key){
                value = encodeURIComponent(value);
                if (!searchString){
                    searchString = '?' + [key, value].join('=');
                }else{
                    searchString += '&' + [key, value].join('=');
                }
            });
            return searchString;
        }

        /**
         * Sorts an array of facets by category then facet
         *
         * @param {array} facetArray ['genre:action', 'genre:adventure', 'gameType:basegame']
         * @returns {Array} sorted array
         */
        function sortFacetArray(facetArray){
            var groupedByCategory = {};
            _.forEach(facetArray, function(facetString){
                var array = facetString.split(':');
                if (array.length === 2){
                    if (!groupedByCategory[array[0]]){
                        groupedByCategory[array[0]] = [];
                    }
                    groupedByCategory[array[0]].push(array[1]);
                }
            });

            var keysSorted = Object.keys(groupedByCategory).sort();
            var sortedArray = [];
            _.forEach(keysSorted, function(key){
                var valueArray = groupedByCategory[key].sort();
                _.forEach(valueArray, function(value){
                    sortedArray.push([key, value].join(':'));
                });
            });
            return sortedArray;
        }

        /**
         * Gets a relative URL with the new facet in query param
         *
         * @param {string} category
         * @param {string} facet
         * @returns {string} URL
         */
        function getUrlWithNewFacet(category, facet){
            var searchString = _.trim(window.location.search, '?'),
                queryParameters = UrlHelper.convertQueryStrToObj(searchString),
                queryParamsSplit,
                newCategoryParam,
                currentPath = $location.path();
            if (category && facet) {
                queryParamsSplit = splitCommaDelimitedFacetQueryParam(queryParameters.fq);
                if (queryParamsSplit.length){
                    newCategoryParam = [category, facet].join(':');
                    queryParamsSplit.push(newCategoryParam);
                    queryParamsSplit = _.uniq(queryParamsSplit);
                    queryParamsSplit = sortFacetArray(queryParamsSplit);
                    queryParameters.fq = queryParamsSplit.join(',');
                }
                return currentPath + getSearchStringFromParameters(queryParameters);
            }
        }

        /**
         * Gets a relative URL with new sort ID
         * @param {string} sortId dynamicPrice asc
         * @returns {string} URL
         */
        function getUrlWithNewSort(sortId){
            var searchString = _.trim(window.location.search, '?'),
                queryParameters = UrlHelper.convertQueryStrToObj(searchString),
                currentPath = $location.path();
            if (sortId){
                queryParameters.sort= sortId;
                return currentPath + getSearchStringFromParameters(queryParameters);
            }
        }

        /**
         * Gets a relative URL with parameter facet removed
         *
         * @param {string} category
         * @param {string} facet
         * @returns {string} URL
         */
        function getUrlWithoutFacet(category, facet){
            var searchString = _.trim(window.location.search),
                queryParameters = UrlHelper.convertQueryStrToObj(searchString),
                queryParamsSplit,
                currentPath = $location.path();
            if (category && facet) {
                queryParamsSplit = splitCommaDelimitedFacetQueryParam(queryParameters.fq);
                if (queryParamsSplit.length){
                    _.remove(queryParamsSplit, function(queryToken){
                        var keyValuePair = queryToken.split(':');
                        if (keyValuePair.length >= 2){
                            var key = keyValuePair[0];
                            var value = keyValuePair[1];
                            return key === category && value === facet;
                        }
                        return false;
                    });
                    queryParamsSplit = sortFacetArray(queryParamsSplit);
                    queryParameters.fq = queryParamsSplit.join(',');
                }
                return currentPath + getSearchStringFromParameters(queryParameters);
            }
        }

        function updateSortValuesFromQuery(sortOrderValue) {
            if (sortOrderValue) {
                sortOrders().setFilterValue(sortOrderValue, true);
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
            updateSortValuesFromQuery: updateSortValuesFromQuery,
            extractFacetValuesFromQuery: extractFacetValuesFromQuery,
            splitCommaDelimitedFacetQueryParam: splitCommaDelimitedFacetQueryParam,
            getUrlWithNewFacet: getUrlWithNewFacet,
            getUrlWithoutFacet: getUrlWithoutFacet,
            getUrlWithNewSort: getUrlWithNewSort
        };
    }

    angular.module('origin-components')
        .factory('StoreSearchQueryFactory', StoreSearchQueryFactory);
}());
