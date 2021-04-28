/**
 * @file store/storefacetcategorygroup.js
 */

(function() {
    'use strict';

    function originStoreSearchFacetCategoryGroup(ComponentsConfigFactory, StoreSearchHelperFactory) {

        function originStoreSearchFacetCategoryGroupLink(scope) {

             function dismissAllFilters() {
                StoreSearchHelperFactory.dismissAllFacets();
                StoreSearchHelperFactory.goToBrowsePage();
            }

            scope.hasActiveFilters = StoreSearchHelperFactory.hasActiveFilters;

            scope.onClick = function () {
                if (scope.hasActiveFilters()) {
                   dismissAllFilters();
                }
            };
        }

        return {
            restrict: 'E',
            replace: true,
            transclude: true,
            scope: {
                title: '@',
                clearAllLabel: '@'
            },
            link: originStoreSearchFacetCategoryGroupLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/storefacetcategorygroup.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreSearchFacets
     * @restrict A
     * @element ANY
     * @scope
     * @param {LocalizedString} title Title of facet group
     * @param {LocalizedString} clear-all-label Label for "clear all filters" label of menu
     * @description
     *
     * Creates a list of facets defined in StoreSearchFactory to be used for narrowing down results in search/browse page
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-facet-category-group title="Filters" clear-all-label="Clear all filters">
     *              <origin-store-facet-category></origin-store-facet-category>
     *         </origin-store-facet-category-group>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .directive('originStoreFacetCategoryGroup', originStoreSearchFacetCategoryGroup);

}());
