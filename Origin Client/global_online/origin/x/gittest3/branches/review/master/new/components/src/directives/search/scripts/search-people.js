(function () {
    'use strict';

    var CONTEXT_NAMESPACE = "origin-global-search";

    function OriginSearchPeopleSectionCtrl($scope, UtilFactory) {

		$scope.sectionTitle = UtilFactory.getLocalizedStr('', CONTEXT_NAMESPACE, 'peopletitle', {
            '%avail%': 0,
            '%total%': 0
        });

        function update() {
            if ($scope.data.results.people) {
                $scope.sectionTitle = UtilFactory.getLocalizedStr('', CONTEXT_NAMESPACE, 'peopletitle', {
                    '%avail%': $scope.data.results.people.length,
                    '%total%': $scope.data.results.people.length
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
     *         <origin-search-people></origin-search-people>
     *     </file>
     * </example>
     */
    function originSearchPeopleSection(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            scope: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('search/views/search-people.html'),
            controller: 'OriginSearchPeopleSectionCtrl'
        };
    }

    angular.module('origin-components')
        .controller('OriginSearchPeopleSectionCtrl', OriginSearchPeopleSectionCtrl)
        .directive('originSearchPeople', originSearchPeopleSection);
}());