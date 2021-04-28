/**
 * @file filter/filtersubmenu.js
 */
(function() {
    'use strict';

    /* jshint ignore:start */
    var CategoryEnumeration = {
        "Genre": "genre",
        "Price": "price",
        "Franchise": "franchise",
        "Game Type": "gameType",
        "Platform": "platform",
        "Availability": "availability",
        "Players": "players",
        "Rating": "rating",
        "Developer": "developer"
        };
    /* jshint ignore:end */
    var DEFAULT_OPEN_CATEGORIES = ['genre', 'price'];

    function OriginStoreFacetCategoryCtrl($scope, StoreSearchHelperFactory, StoreSearchFactory) {

        $scope.open = false;

        function isOpen() {
            return (DEFAULT_OPEN_CATEGORIES.indexOf($scope.categoryId) >=0 || StoreSearchHelperFactory.hasActiveFilters($scope.categoryId));
        }

        var stopWatchingModel = $scope.$watch(StoreSearchFactory.isModelReady, function (newValue) {
            if (newValue) {
                $scope.open = isOpen();
                //Need to watch for the very first time,
                //then don't check in case user is looking at facets accidentally triggers infinite scroll search.
                stopWatchingModel();
            }

        });

        /**
         * Toggles open/closed state.
         * @return {void}
         */
        $scope.toggle = function () {
            $scope.open = !$scope.open;
        };
    }

    function originStoreFacetCategory(ComponentsConfigFactory) {

        return {
            restrict: 'E',
            transclude: true,
            replace: true,
            scope: {
                label: '@',
                categoryId: '@'
            },
            controller: 'OriginStoreFacetCategoryCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/storefacetcategory.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreFacetCategory
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} label of the sub menu
     * @param {CategoryEnumeration} category-id Type of this category. Must be an enum
     * @description
     *
     * Filter panel item that can be toggled on and off.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-facet-category label="Genre" categoryId="Genre"></origin-store-facet-category>
     *     </file>
     * </example>
     *
     */

        // directive declaration
    angular.module('origin-components')
        .controller('OriginStoreFacetCategoryCtrl', OriginStoreFacetCategoryCtrl)
        .directive('originStoreFacetCategory', originStoreFacetCategory);
}());
