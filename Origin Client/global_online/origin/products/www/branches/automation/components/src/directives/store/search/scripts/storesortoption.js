/**
 * @file filter/filteroption.js
 */
(function () {
    'use strict';

    function originStoreSortOption(ComponentsConfigFactory, StoreSearchHelperFactory, StoreFacetFactory, AppCommFactory, StoreSearchQueryFactory, NavigationFactory) {
        function originStoreSortOptionLink(scope) {
            scope.countVisible = false;
            var categoryId = 'sort';

            StoreFacetFactory.registerSortOption(scope.sortId);

            /**
             * Determines whether the sort is selected or not.
             */
            function setActiveStatus() {
                scope.isActive = StoreSearchHelperFactory.isFilterActive(categoryId, scope.sortId);
                populateLink();
            }

            function onDestroy() {
                AppCommFactory.events.off('storesearch:completed', setActiveStatus);
            }

            scope.enabled = function () {
                return true;
            };

            /**
             * Toggles active/inactive state. Doesn't affect disabled items
             * @return {void}
             */
            scope.toggle = function () {
                if (!scope.isActive) {
                    scope.isActive = !scope.isActive;
                    AppCommFactory.events.fire('filterpanel:toggle');

                    StoreSearchHelperFactory.applySortFilter(categoryId, scope.sortId);
                }
            };

            function populateLink(){
                if (scope.isActive){
                    scope.filterUrl = '';
                }else{
                    scope.filterUrl = NavigationFactory.getAbsoluteUrl(StoreSearchQueryFactory.getUrlWithNewSort(encodeURIComponent(scope.sortId)));
                }
            }

            AppCommFactory.events.on('storesearch:completed', setActiveStatus);

            scope.$on('$destroy', onDestroy);
        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
                label: '@',
                sortId: '@'
            },
            link: originStoreSortOptionLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/search/views/storefacetoption.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreSortOption
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} label of the sort option
     * @param {string} sort-id Type of this sort eg 'rank desc', 'dynamicPrice asc', 'title asc'
     * @description
     *
     * Filter panel item that can be toggled on and off.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-sort-category label="Genre" category-id="genre">
     *           <origin-store-sort-option label="Action" sort-id="action"></origin-store-sort-option>
     *         </origin-store-sort-category>
     *     </file>
     * </example>
     *
     */

        // directive declaration
    angular.module('origin-components')
        .directive('originStoreSortOption', originStoreSortOption);
}());

