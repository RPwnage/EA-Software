/**
 * @file filter/filteroption.js
 */
(function() {
    'use strict';
    /* jshint ignore:start */
    var SortIdEnumeration = {
            "Most Popular": "rank desc",
            "Price - High to Low": "dynamicPrice asc",
            "Price - Low to High": "dynamicPrice desc",
            "Release Date": "releaseDate desc",
            "Alphabetical - A to Z": "title asc",
            "Alphabetical - Z to A": "title desc"
        };
        /* jshint ignore:end */


    function originStoreSortOption(ComponentsConfigFactory, StoreSearchHelperFactory, StoreSearchFactory) {
        function originStoreSortOptionLink(scope) {
            scope.countVisible = false;
            var categoryId = 'sort';

            /**
             * Determines whether the sort is selected or not.
             */
            function setActiveStatus(newValue) {
                if (newValue) {
                    scope.isActive = StoreSearchHelperFactory.isFilterActive(categoryId, scope.sortId);
                }
            }

            scope.$watch(StoreSearchFactory.isModelReady, setActiveStatus);


            scope.enabled = function () {
                return true;
            };

            /**
             * Toggles active/inactive state. Doesn't affect disabled items
             * @return {void}
             */
            scope.toggle = function () {
                if (scope.enabled() && !scope.isActive) {
                    scope.isActive = !scope.isActive;
                    StoreSearchHelperFactory.applyFilter(categoryId, scope.sortId);
                }

            };
        }

        return {
            restrict: 'E',
            scope: {
                label: '@',
                sortId: '@'
            },
            link: originStoreSortOptionLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/storefacetoption.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreSortOption
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} label of the sort option
     * @param {SortIdEnumeration} sort-id Type of this sort. Must be an enum
     * @description
     *
     * Filter panel item that can be toggled on and off.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-sort-category label="Genre" categoryId="genre">
     *           <origin-store-sort-option label="Action" sortId="action"></origin-store-sort-option>
     *         </origin-store-sort-category>
     *     </file>
     * </example>
     *
     */

        // directive declaration
    angular.module('origin-components')
        .directive('originStoreSortOption', originStoreSortOption);
}());

