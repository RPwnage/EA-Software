(function () {
    'use strict';

    var CONTEXT_NAMESPACE = "origin-global-search";

    function OriginSearchGamesSectionCtrl($scope, UtilFactory) {

		$scope.sectionTitle = UtilFactory.getLocalizedStr('', CONTEXT_NAMESPACE, 'gamestitle', {
            '%avail%': 0,
            '%total%': 0
        });

        function update() {
            if ($scope.data.results.games) {
                $scope.sectionTitle = UtilFactory.getLocalizedStr('', CONTEXT_NAMESPACE, 'gamestitle', {
                    '%avail%': $scope.data.results.games.length,
                    '%total%': $scope.data.results.games.length
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
     *         <origin-search-games></origin-search-games>
     *     </file>
     * </example>
     */
    function originSearchGamesSection(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            scope: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('search/views/search-games.html'),
            controller: 'OriginSearchGamesSectionCtrl'
        };
    }

    angular.module('origin-components')
        .controller('OriginSearchGamesSectionCtrl', OriginSearchGamesSectionCtrl)
        .directive('originSearchGames', originSearchGamesSection);
}());