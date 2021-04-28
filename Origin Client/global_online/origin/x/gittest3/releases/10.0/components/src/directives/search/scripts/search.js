(function () {
    'use strict';

    var CONTEXT_NAMESPACE = "origin-global-search";

    function OriginSearchPageCtrl($scope, $routeParams, UtilFactory, SearchFactory) {

        $scope.data = SearchFactory.createSearchContext();
		$scope.searchString = decodeURI($routeParams.searchString);

		$scope.searchPageTitle = UtilFactory.getLocalizedStr('', CONTEXT_NAMESPACE, 'searchpagetitle', {
            '%searchString%': $scope.searchString
        });

        // This needs to deal with includes/excludes in the route params.
        SearchFactory.search($scope.data, {
            searchString: $scope.searchString,
            includes: $routeParams.includes,
            excludes: $routeParams.excludes,
            store: {
                fq: 'gameType:basegame',
                sort: 'title asc',
                start: 0,
                rows: 20
            }
        });

        function update() {
            $scope.searchPageTitle = UtilFactory.getLocalizedStr('', CONTEXT_NAMESPACE, 'searchpagetitle', {
                '%searchString%': $scope.searchString
            });
            $scope.$digest();
        }

        function onDestroy() {
            $scope.data.events.off('search:started', update);
            $scope.data.events.off('search:updated', update);
            $scope.data.events.off('search:finished', update);
        }

        $scope.data.events.on('search:started', update);
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
     *         <origin-search-page></origin-search-page>
     *     </file>
     * </example>
     */
    function originSearchPage(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            scope: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('search/views/search.html'),
            controller: 'OriginSearchPageCtrl'
        };
    }

    angular.module('origin-components')
        .controller('OriginSearchPageCtrl', OriginSearchPageCtrl)
        .directive('originSearchPage', originSearchPage);
}());