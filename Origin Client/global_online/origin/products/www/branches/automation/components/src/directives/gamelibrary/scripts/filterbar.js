/**
 * @file gamelibrary/filterbar.js
 */
(function() {
    'use strict';

    function originGamelibraryFilterBar(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                onChange: '&'
            },
            controller: 'OriginGamelibraryFilterCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/filterbar.html'),
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryFilterBar
     * @restrict E
     * @element ANY
     * @scope
     * @param {function} on-change function to call when any of the filters change
     * @description
     *
     * Game library-specific active filter bar.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-gamelibrary-filter-bar on-change="refreshView()"></origin-gamelibrary-filter-bar>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .directive('originGamelibraryFilterBar', originGamelibraryFilterBar);
}());
