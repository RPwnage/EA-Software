(function () {
    'use strict';

    var CONTEXT_NAMESPACE = "origin-global-search";

    function OriginSearchStoreSectionCtrl($scope, UtilFactory) {

        $scope.sectionTitle = UtilFactory.getLocalizedStr('', CONTEXT_NAMESPACE, 'storetitle', {
            '%avail%': 0,
            '%total%': 0
        });

        function update() {
            if ($scope.data.results.store) {
                $scope.sectionTitle = UtilFactory.getLocalizedStr('', CONTEXT_NAMESPACE, 'storetitle', {
                    '%avail%': $scope.data.results.store.numVisible,
                    '%total%': $scope.data.results.store.numFound
                });
            }
        }

        function onDestroy() {
            $scope.data.events.off('search:updated', update);
            $scope.data.events.off('search:finished', update);
        }

        $scope.data.events.on('search:updated', update);
        $scope.data.events.on('search:finished', update);
        $scope.$on('$destroy', onDestroy);
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
     *         <origin-search-store></origin-search-store>
     *     </file>
     * </example>
     */
    function originSearchStoreSection(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            scope: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('search/views/search-store.html'),
            controller: 'OriginSearchStoreSectionCtrl'
        };
    }

    angular.module('origin-components')
        .controller('OriginSearchStoreSectionCtrl', OriginSearchStoreSectionCtrl)
        .directive('originSearchStore', originSearchStoreSection);
}());