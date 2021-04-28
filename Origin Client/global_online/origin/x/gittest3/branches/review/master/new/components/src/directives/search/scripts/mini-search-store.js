(function () {
    'use strict';

    var CONTEXT_NAMESPACE = "origin-global-search";
    
    function OriginMiniSearchStoreSectionCtrl($scope, 
                                                UtilFactory, 
                                                SearchFactory,
                                                GamesCatalogFactory) {
        
		$scope.sectionTitle = UtilFactory.getLocalizedStr('', CONTEXT_NAMESPACE, 'gamestitle');
        $scope.catalog = GamesCatalogFactory.getCatalog();
    }
        
    
    /**
     * @ngdoc directive
     * @name origin-components.directives:originSearch
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * main search page.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-search-games></origin-search-games>
     *     </file>
     * </example>
     */
    function originMiniSearchStoreSection(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            scope: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('search/views/mini-search-store.html'),
            controller: 'OriginMiniSearchStoreSectionCtrl'
        };
    }

    angular.module('origin-components')
        .controller('OriginMiniSearchStoreSectionCtrl', OriginMiniSearchStoreSectionCtrl)
        .directive('originMiniSearchStore', originMiniSearchStoreSection);
}());