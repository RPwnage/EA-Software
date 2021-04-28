/**
 * @file shell/scripts/globalsearch.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-shell-global-search';

    /**
     * Controller Global Search Ctrl
     */
    function OriginShellGlobalSearchCtrl($scope, DialogFactory, ComponentsConfigFactory, UtilFactory) {

        // TODO: REMOVE THIS
        var localeMap = {
            'en_US': ComponentsConfigFactory.getImagePath('canada-small.png'),
            'pl_PL': ComponentsConfigFactory.getImagePath('poland-small.png')
        };

        $scope.searchcta = UtilFactory.getLocalizedStr($scope.searchctaStr, CONTEXT_NAMESPACE, 'searchcta');
        $scope.locale = Origin.locale.locale();
        $scope.localeImg = localeMap[$scope.locale];

        /**
         * Show the change language dialog
         * @method
         */
        $scope.changeLanguage = function() {
            DialogFactory.openDirective({
                id: 'web-change-language', 
                name: 'origin-dialog-content-changelanguage'
            });
        };

    }

    /**
     * Global Search Directive
     */
    function originShellGlobalSearch(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                searchctaStr: '@searchcta'
            },
            controller: 'OriginShellGlobalSearchCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('shell/views/globalsearch.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originShellGlobalSearch
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} searchcta the search call to action
     * @description
     *
     * global search
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-shell-global-search></origin-shell-global-search>
     *     </file>
     * </example>
     *
     */
    // declaration
    angular.module('origin-components')
        .controller('OriginShellGlobalSearchCtrl', OriginShellGlobalSearchCtrl)
        .directive('originShellGlobalSearch', originShellGlobalSearch);

}());