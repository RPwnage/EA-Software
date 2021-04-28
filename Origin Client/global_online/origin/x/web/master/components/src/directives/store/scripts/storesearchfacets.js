/**
 * @file store/storesearchfacets.js
 */

(function() {
    'use strict';

    function OriginStoreSearchFacetsCtrl($scope) {


        $scope.isVisible = function() {
            return true;
        };

    }

    function originStoreSearchFacets(ComponentsConfigFactory) {

        return {
            restrict: 'E',
            replace: true,
            transclude: true,
            scope: {},
            controller: OriginStoreSearchFacetsCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/storesearchfacets.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreSearchFacets
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * Creates a list of facets defined in StoreSearchFactory to be used for narrowing down results in search/browse page
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-search-facets></origin-store-search-facets>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginStoreSearchFacetsCtrl', OriginStoreSearchFacetsCtrl)
        .directive('originStoreSearchFacets', originStoreSearchFacets);

}());
