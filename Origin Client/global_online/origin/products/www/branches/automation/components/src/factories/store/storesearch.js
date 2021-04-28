/**
 * @file store/storesearch.js
 */
(function() {
    'use strict';

    var DEFAULT_SORT_ORDER = 'rank desc';
    var CUSTOM_SORT_START = 'customRank_';
    var CUSTOM_RANK_ORDER = ' desc';
    var CUSTOM_ORDER_REGEX = new RegExp('^' + DEFAULT_SORT_ORDER);

    function StoreSearchFactory(ObjectHelperFactory, StoreSearchHelperFactory, SearchFactory, StoreFacetFactory, StoreSearchQueryFactory, ComponentsLogFactory, PageThinkingFactory, AppCommFactory) {
        var o = ObjectHelperFactory,
            ssqf = StoreSearchQueryFactory,
            sshf = StoreSearchHelperFactory,
            searchString = '',
            sortOrder = DEFAULT_SORT_ORDER,
            fq = '',
            fqOverride = '',
            offset = 0,
            limit = 0,
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
            searchString = '';
            sortOrder = DEFAULT_SORT_ORDER;
            fq = '';
            fqOverride = '';
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

            ssqf.updateFacetValuesFromQuery(fq);
            return this;
        }

        /**
         * Sets fq (facets) override parameter for search that will be applied as default filter
         *
         * @param {string} facets - facet category ID
         *
         * @return {this}
         */
        function setFacetOverride(facets) {
            fqOverride = facets || '';
            return this;
        }

        function setCustomSort(customOrder) {
            if(customOrder && CUSTOM_ORDER_REGEX.test(sortOrder)) {
                customOrder = CUSTOM_SORT_START + customOrder + CUSTOM_RANK_ORDER;
                sortOrder = customOrder + ',' + sortOrder;
            }
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
         * @param {Number} newValue search results limit
         * @return {this}
         */
        function setLimit(newValue) {
            limit = newValue;

            return this;
        }

        /**
         * Sets search results offset
         *
         * @param {Number} newValue search results offset
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
            var facets = StoreFacetFactory.getFacets();
            _.forEach(data.facets, function (dataFacet) {
                if (dataFacet.constraint) {
                    facets[dataFacet.facetName].setAllFilterCounts(dataFacet.constraint);
                }
            });
            return data;
        }

        /**
         * Returns combined filter query string from facet query and facet query override
         * @returns {string}
         */
        function getFilterQuery() {
            var params = _.compact([fq, fqOverride]);
            return params.length > 0 ? constructFilterQuery(params.join(',')) : '';
        }

        /**
         * Construct valid filter query for xSearch service, combining multiple facets
         * in the same group with "OR"
         * @param  {string} queryString query string
         * @return {string} string to use for xSearch filterQuery value
         */
        function constructFilterQuery(queryString) {
            var queryParams = _.map(ssqf.splitCommaDelimitedFacetQueryParam(queryString), function(val) {
                return val.split(':');
            });
            var mergedQueryParams = o.mergeToObject(queryParams);

            return _.map(Object.keys(mergedQueryParams), function(key) {
                var length = mergedQueryParams[key].length;
                if (length > 1) {
                    // If there is more than one facet selected for this category,
                    // we need to "OR" them so any one of them could match, not necessarily all.
                    return key + ':(' + mergedQueryParams[key].join(' AND ') + ')';
                } else if (length === 1) {
                    // If there's only one facet, set it directly
                    return key + ':' + mergedQueryParams[key][0];
                } else {
                    // No facet?  Discard this as it's not valid for search query
                    return '';
                }
            });
        }

        function storeSearch() {
            var searchParams = {
                filterQuery: getFilterQuery(),
                facetField: ssqf.getFacetCategories(),
                sort: sortOrder,
                start: offset,
                rows: limit
            };
            return SearchFactory.searchStore(searchString, searchParams);
        }

        /**
         * @return {Promise}
         */
        function search() {
            return storeSearch()
                .then(function(data) {
                    return {
                        totalResults: extractPropertyOrDefault(data, ['games', 'numFound'], 0),
                        games: extractPropertyOrDefault(data, ['games', 'game'], []),
                        facets: extractPropertyOrDefault(data, ['facetList', 'facets'], [])
                    };
                })
                .then(updateFacetCount)
                .then(function(data) {
                    AppCommFactory.events.fire('storesearch:completed', offset > 0); // argument to indicate whether this was a pagination/lazy-load search
                    return data;
                })
                .catch(PageThinkingFactory.unblockUIPromiseOnError)
                .catch(function (error) {
                    ComponentsLogFactory.error('[StoreSearchFactory: SearchFactory.searchStore] failed', error);
                    return Promise.reject(error);
                });
        }


        return {
            reset: reset,
            setFacets: setFacets,
            setFacetOverride: setFacetOverride,
            setSearchString: setSearchString,
            setCustomSort: setCustomSort,
            setSortBy: setSortBy,
            setLimit: setLimit,
            setOffset: setOffset,
            search: search
        };

    }

    angular.module('origin-components')
        .factory('StoreSearchFactory', StoreSearchFactory);
}());
