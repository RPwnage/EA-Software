/**
 * @file shell/scripts/globalsearch.js
 */
(function () {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-global-search';
    /**
     * Controller Global Search Ctrl
     */
    function OriginGlobalSearchCtrl($scope,
                                    UtilFactory,
                                    SearchFactory) {

        $scope.data = SearchFactory.createSearchContext();

        // Register to the search providers. Move this to some other location, and read the configuration from a data source.
        $scope.searchPlaceholder = UtilFactory.getLocalizedStr('', CONTEXT_NAMESPACE, 'searchplaceholder');


        function onSearchStarted() {
            $scope.dropdownVisible = true;
        }

        function onSearchUpdated() {
            $scope.$digest();
        }

        function onDestroy() {
            $scope.data.events.off('search:started', onSearchStarted);
            $scope.data.events.off('search:updated', onSearchUpdated);
        }

        $scope.data.events.on('search:started', onSearchStarted);
        $scope.data.events.on('search:updated', onSearchUpdated);
        $scope.$on('$destroy', onDestroy);

        $scope.dropdownVisible = false;
        $scope.quickSearchActive = true;

        $scope.search = function (fullSearch, includes) {
            if ($scope.searchString !== '') {
                if (fullSearch) {
                    $scope.quickSearchActive = false;
                    $scope.dropdownVisible = false;
                    window.location.href = '/#/search?searchString=' + encodeURI($scope.searchString) + ((includes !== undefined) ? '&includes=' + includes : '');
                } else {
                    $scope.quickSearchActive = true;
                    SearchFactory.search($scope.data, {
                        searchString: $scope.searchString,
                        includes: $scope.includes,
                        store: {
                            fq: 'gameType:basegame',
                            sort: 'title asc',
                            start: 0,
                            rows: 3
                        }
                    });
                }
            } else {
                $scope.dropdownVisible = false;
            }
        };

        $scope.clicked = function (url, item) {
            window.location.href = url + item;
            $scope.dropdownVisible = false;
        };

        $scope.openbox = function () {
            if ($scope.searchString !== '' && $scope.searchString !== undefined) {
                $scope.dropdownVisible = true;
            }
        };
    }

    /**
     * Global Search Directive
     */
    function originGlobalSearch(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            controller: 'OriginGlobalSearchCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('shell/views/search.html'),
            scope: {
                includes: '@includes',
                excludes: '@excludes',
                source: '@source'
            }
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originShellGlobalSearch
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * global search
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-global-search></origin-global-search>
     *     </file>
     * </example>
     *
     */
    // declaration
    angular.module('origin-components')
        .controller('OriginGlobalSearchCtrl', OriginGlobalSearchCtrl)
        .directive('originGlobalSearch', originGlobalSearch);

}());