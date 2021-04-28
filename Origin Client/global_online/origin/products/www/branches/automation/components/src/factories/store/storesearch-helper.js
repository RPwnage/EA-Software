/**
 * @file store/storesearch-helper.js
 */
(function () {
    'use strict';

    function StoreSearchHelperFactory($state, StoreFacetFactory, StoreSearchQueryFactory, AppCommFactory) {

        /**
         * Dismisses all facets registered in StoreFacetFactory
         */
        function dismissAllFacets() {
            _.forEach(StoreFacetFactory.getFacets(), function (facet) {
                facet.dismissAll();
            });
        }

        /**
         * Dismisses all sortOrders registered in StoreFacetFactory
         */
        function dismissAllSortOrders() {
            var order = StoreFacetFactory.getSortOrders();
            if(order){
                order.dismissAll();
            }
        }

        /**
         * Given categoryId (genre, etc) has any active filters
         * @param categoryId
         * @returns {Boolean}
         */
        function hasActiveFilters(categoryId) {
            if (categoryId) {
                return StoreFacetFactory.getFacets()[categoryId].countActiveFilters() > 0;
            }
            return false;
        }

        /**
         * Given categoryId (genre, etc) has any filter with count > 0
         * @param categoryId
         * @returns {Boolean}
         */
        function hasSelectableFilters(categoryId) {
            if (categoryId) {
                return StoreFacetFactory.getFacets()[categoryId].isFilterSelectableAndVisible();
            }
            return false;
        }

        /**
         * Returns total active facets count
         * @returns {number} activeFacetsCount
         */
        function getActiveFacetsCount(){
            var facets = StoreFacetFactory.getFacets(),
                facetsKeys = Object.keys(facets),
                activeFacetsCount = _.reduce(facetsKeys, function(sum, categoryId){
                return sum += facets[categoryId].countActiveFilters();
            }, 0);
            return activeFacetsCount;
        }

        /**
         * is any filter active across all filters
         * @param categoryId
         * @returns {*}
         */
        function isAnyFilterActive() {
            var categories = Object.keys(StoreFacetFactory.getFacets());
            var result = _.find(categories, function (category) {
                return hasActiveFilters(category);
            });

            return angular.isDefined(result);
        }

        /**
         * Returns the sort group or facet group based on categoryId
         * @param categoryId
         * @returns {*}
         */
        function getFilterGroupByType(categoryId) {
            if (categoryId !== 'sort') {
                return StoreFacetFactory.getFacets()[categoryId];
            }
            return StoreFacetFactory.getSortOrders();
        }

        /**
         * Is filter with given category/filterName active
         * @param categoryId
         * @param filterName
         * @returns {*|boolean}
         */
        function isFilterActive(categoryId, filterName) {
            return getFilterGroupByType(categoryId).isFilterActive(filterName);
        }

        /**
         * Returns all values of a given filter
         * @param categoryId
         * @param filterName
         * @returns {*|Object}
         */
        function getFilter(categoryId, filterName) {
            var facets = StoreFacetFactory.getFacets();
            return facets[categoryId] && facets[categoryId].getFilter(filterName);
        }

        /**
         * Toggles the value of a facet
         * @param categoryId
         * @param filterName
         */
        function toggleFacet(categoryId, filterName) {
            var facetGroup = getFilterGroupByType(categoryId);
            var isActive = facetGroup.isFilterActive(filterName);
            facetGroup.setFilterValue(filterName, !isActive);
        }

        /**
         * Updates url based on currently selected facet/sort options
         */
        function updateURL() {
            return $state.go($state.current.name, {
                searchString: StoreSearchQueryFactory.getSearchStringQueryParam(),
                fq: StoreSearchQueryFactory.getActiveFacetsAsString(),
                sort: StoreSearchQueryFactory.getSortAsString()
            }, {notify: false});
        }

        function triggerSearch() {
            AppCommFactory.events.fire('storeSearchHelper:triggerSearch');
        }

        /**
         * Update facet values and trigger search
         * @param categoryName
         * @param filterName
         */
        function applyFilter(categoryName, filterName) {
            if (categoryName && filterName) {
                toggleFacet(categoryName, filterName);
                updateURL().then(triggerSearch);
            }
        }

        function applySortFilter(categoryName, filterName) {
            dismissAllSortOrders();
            applyFilter(categoryName, filterName);
        }

        return {
            dismissAllFacets: dismissAllFacets,
            dismissAllSortOrders: dismissAllSortOrders,
            hasActiveFilters: hasActiveFilters,
            hasSelectableFilters: hasSelectableFilters,
            getFilter: getFilter,
            applyFilter: applyFilter,
            applySortFilter: applySortFilter,
            reloadCurrentPage: updateURL,
            isFilterActive: isFilterActive,
            isAnyFilterActive: isAnyFilterActive,
            triggerSearch: triggerSearch,
            getActiveFacetsCount: getActiveFacetsCount
        };

    }

    angular.module('origin-components')
        .factory('StoreSearchHelperFactory', StoreSearchHelperFactory);
}());
