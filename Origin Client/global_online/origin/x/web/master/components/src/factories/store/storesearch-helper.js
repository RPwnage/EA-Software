/**
 * @file store/storesearch-helper.js
 */
(function() {
    'use strict';

    function StoreSearchHelperFactory($state, StoreFacetFactory, StoreSearchQueryFactory, ObjectHelperFactory) {

        var facets = StoreFacetFactory.facets,
            sortOrders = StoreFacetFactory.sortOrders,
            ssqf = StoreSearchQueryFactory,
            ohf = ObjectHelperFactory;

        /**
         * Dismisses all facets registered in StoreFacetFactory
         */
        function dismissAllFacets() {
            ohf.forEach(function(facet) {
                facet.dismissAll();
            }, facets);
        }

        /**
         * Dismisses all sortOrders registered in StoreFacetFactory
         */
        function dismissAllSortOrders() {
            sortOrders.dismissAll();
        }

        /**
         * Given categoryId (genre, etc) has any active filters
         * @param categoryId
         * @returns {*}
         */
        function hasActiveFilters(categoryId) {
            if (categoryId) {
                return facets[categoryId].countActiveFilters() > 0;
            } else {
                return ohf.find(function(facet) {
                    return facet.countActiveFilters() > 0;
                }, facets);
            }
        }

        /**
         * Returns the sort group or facet group based on categoryId
         * @param categoryId
         * @returns {*}
         */
        function getFilterGroupByType(categoryId) {
            if (categoryId !== 'sort') {
                return facets[categoryId];
            }
            return sortOrders;
        }

        /**
         * Is filter with given category/filterName active
         * @param categoryId
         * @param filterName
         * @returns {*|boolean}
         */
        function isFilterActive (categoryId, filterName) {
            return getFilterGroupByType(categoryId).isFilterActive(filterName);
        }

        /**
         * Returns all values of a given filter
         * @param categoryId
         * @param filterName
         * @returns {*|Object}
         */
        function getFilter(categoryId, filterName) {
            return facets[categoryId].getFilter(filterName);
        }

        /**
         * Toggles the value of a facet
         * @param categoryId
         * @param filterName
         */
        function toggleFacet(categoryId, filterName) {

            var facetGroup = getFilterGroupByType(categoryId);

            var isActive = facetGroup.isFilterActive(filterName);
            facetGroup.dismissAll();
            facetGroup.setFilterValue(filterName, !isActive);
        }

        /**
         * Updates url based on currently selected facet/sort options
         */
        function goToBrowsePage() {
            console.log('goToBrowsePage');
            $state.go('app.store.wrapper.browse', {
                searchString : ssqf.getSearchStringQueryParam(),
                fq: ssqf.getActiveFacetsAsString(),
                sort: ssqf.getSortAsString()
            });
        }

        /**
         * Update facet values and trigger search
         * @param categoryName
         * @param filterName
         */
        function applyFilter(categoryName, filterName) {
            if (categoryName && filterName) {
                toggleFacet(categoryName, filterName);
                goToBrowsePage();
            }
        }

        return {
            dismissAllFacets: dismissAllFacets,
            dismissAllSortOrders: dismissAllSortOrders,
            hasActiveFilters: hasActiveFilters,
            getFilter: getFilter,
            applyFilter: applyFilter,
            goToBrowsePage: goToBrowsePage,
            isFilterActive: isFilterActive
        };

    }

    angular.module('origin-components')
        .factory('StoreSearchHelperFactory', StoreSearchHelperFactory);
}());
