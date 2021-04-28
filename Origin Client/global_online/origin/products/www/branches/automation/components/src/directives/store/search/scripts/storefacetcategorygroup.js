/**
 * @file store/storefacetcategorygroup.js
 */

(function() {
    'use strict';

    function originStoreSearchFacetCategoryGroup(ComponentsConfigFactory, StoreSearchHelperFactory) {

        function originStoreSearchFacetCategoryGroupLink(scope) {

            function dismissAllFilters() {
                StoreSearchHelperFactory.dismissAllFacets();
                StoreSearchHelperFactory.reloadCurrentPage().then(StoreSearchHelperFactory.triggerSearch);
            }

            scope.isAnyFilterActive = StoreSearchHelperFactory.isAnyFilterActive;

            scope.onClick = function () {
                if (scope.isAnyFilterActive()) {
                    dismissAllFilters();
                }
            };
        }

        return {
            restrict: 'E',
            replace: true,
            transclude: true,
            scope: {
                titleStr: '@',
                clearAllLabel: '@'
            },
            link: originStoreSearchFacetCategoryGroupLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/search/views/storefacetcategorygroup.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreFacetCategoryGroup
     * @restrict A
     * @element ANY
     * @scope
     * @param {LocalizedString} title-str Title of facet group
     * @param {LocalizedString} clear-all-label Label for "clear all filters" label of menu
     * @description
     *
     * Creates a list of facets defined in StoreSearchFactory to be used for narrowing down results in search/browse page
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-facet-category-group title-str="Filters" clear-all-label="Clear all filters">
     *              <origin-store-facet-category></origin-store-facet-category>
     *         </origin-store-facet-category-group>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .directive('originStoreFacetCategoryGroup', originStoreSearchFacetCategoryGroup);

}());
