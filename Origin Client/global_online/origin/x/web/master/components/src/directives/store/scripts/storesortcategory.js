/**
 * @file filter/filtersubmenu.js
 */
(function() {
    'use strict';

    function OriginStoreSortCategoryCtrl($scope) {

        $scope.categoryId = 'sort';
        $scope.open = true;


        /**
         * Toggles open/closed state.
         * @return {void}
         */
        $scope.toggle = function () {
            $scope.open = !$scope.open;
        };
    }

    function originStoreSortCategory(ComponentsConfigFactory) {

        return {
            restrict: 'E',
            transclude: true,
            replace: true,
            scope: {
                label: '@'
            },
            controller: 'OriginStoreSortCategoryCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/storesortcategory.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreSortCategory
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} label of the sub menu
     * @description
     *
     * Filter panel item that can be toggled on and off.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-sort-category label="Sort"></origin-store-sort-category>
     *     </file>
     * </example>
     *
     */

        // directive declaration
    angular.module('origin-components')
        .controller('OriginStoreSortCategoryCtrl', OriginStoreSortCategoryCtrl)
        .directive('originStoreSortCategory', originStoreSortCategory);
}());
