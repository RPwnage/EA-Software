/**
 * @file store/storesearch.js
 */
(function() {
    'use strict';

    var DEFAULT_SORT_ORDER = 'rank desc';

    function StoreSearchFactory(ObjectHelperFactory, StoreSearchHelperFactory, SearchFactory, StoreFacetFactory, StoreSearchQueryFactory, ComponentsLogFactory) {
        var o = ObjectHelperFactory,
            ssqf = StoreSearchQueryFactory,
            sshf = StoreSearchHelperFactory,
            searchString = '',
            sortOrder = DEFAULT_SORT_ORDER,
            facets = StoreFacetFactory.facets,
            fq = '',
            ff = '',
            offset = 0,
            limit = 0,
            isDataModelReady = false,
            sortMap = {
                'rank': ['releaseDate desc', 'title desc'],
                'releaseDate' : ['rank desc', 'title desc'],
                'dynamicPrice' : ['rank desc', 'releaseDate desc', 'title desc'],
                'title' : ['rank desc', 'releaseDate desc']
            };


        function appendSecondarySort(primarySort) {
            var secondarySort = [primarySort],
                sortKeys = Object.keys(sortMap);

            for (var i = 0; i < sortKeys.length; i++) {
                var sortKey = sortKeys[i];
                if (primarySort.indexOf(sortKey) >= 0) {
                    secondarySort = secondarySort.concat(sortMap[sortKey]);
                    break;
                }
            }

            return secondarySort.join(',');
        }



        /**
         * resets all facet values
         * @returns {this}
         */
        function reset() {
            isDataModelReady = false;
            searchString = '';
            sortOrder = DEFAULT_SORT_ORDER;
            fq = '';
            ff = '';
            sshf.dismissAllFacets();
            sshf.dismissAllSortOrders();
            return this;
        }

        /**
         * Sets search string parameter
         *
         * @param {string} newValue new search string
         * @return {this}
         */
        function setSearchString(newValue) {
            searchString = newValue;
            return this;
        }

        /**
         * Sets fq (facets) parameter for search
         *
         * @param {string} newValue new search string
         * @return {this}
         */
        function setFacets(newValue) {
            fq = newValue;

            ff = ssqf.getFacetCategories();

            ssqf.updateFacetValuesFromQuery(fq);
            return this;
        }


        /**
         * Sets sort (sort order) parameter for search
         *
         * @param {string} sortByValue new search string
         * @return {this}
         */
        function setSortBy(sortByValue) {
            sortByValue = sortByValue || DEFAULT_SORT_ORDER;
            ssqf.updateSortValuesFromQuery(sortByValue);
            sortOrder = appendSecondarySort(sortByValue);
            return this;
        }

        /**
         * Sets search results limit
         *
         * @param {integer} newValue search results limit
         * @return {this}
         */
        function setLimit(newValue) {
            limit = newValue;

            return this;
        }

        /**
         * Sets search results offset
         *
         * @param {integer} newValue search results offset
         * @return {this}
         */
        function setOffset(newValue) {
            offset = newValue;

            return this;
        }

        function extractPropertyOrDefault(data, propertyArray, defaultValue) {
            return o.defaultTo(defaultValue)(o.getProperty(propertyArray)(data));
        }

        /**
         * Update filter count
         * @param data
         * @returns {*}
         */
        function updateFacetCount(data) {
            angular.forEach(data.facets, function (dataFacet) {
                if (dataFacet.constraints) {
                    facets[dataFacet.name].setAllFilterCounts(dataFacet.constraints.constraint);
                }
            });
            return data;
        }

        /**
         * @return {Promise}
         */
        function search() {
            var searchParams = {
                fq: fq,
                ff: ff,
                sort: sortOrder,
                start: offset,
                rows: limit
            };

            return SearchFactory.searchStore(searchString, searchParams)
                .then(function(data) {
                    return {
                        totalResults: extractPropertyOrDefault(data, ['result', 'games', 'numFound'], 0),
                        games: extractPropertyOrDefault(data,['result', 'games', 'game'], []),
                        facets: extractPropertyOrDefault(data, ['result', 'facets', 'facet'], [])
                    };
                })
                .then(updateFacetCount)
                .then(function(data) {
                    isDataModelReady = true;
                    return data;
                })
                .catch(function (error) {
                    ComponentsLogFactory.error('[StoreSearchFactory: SearchFactory.searchStore] failed', error.message);
                });
        }

        function isModelReady() {
            return isDataModelReady;
        }

        return {
            reset: reset,
            setFacets: setFacets,
            setSearchString: setSearchString,
            setSortBy: setSortBy,
            setLimit: setLimit,
            setOffset: setOffset,
            search: search,
            isModelReady: isModelReady
        };

    }

    angular.module('origin-components')
        .factory('StoreSearchFactory', StoreSearchFactory);
}());
