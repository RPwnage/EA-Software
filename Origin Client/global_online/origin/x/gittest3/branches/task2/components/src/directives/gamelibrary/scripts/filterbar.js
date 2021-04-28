/**
 * @file gamelibrary/filterbar.js
 */
(function() {
    'use strict';

    function originGamelibraryFilterBar(ComponentsConfigFactory) {
        return {
            restrict: 'E',
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
     * @description
     *
     * Game library-specific active filter bar.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-gamelibrary-filter-bar></origin-gamelibrary-filter-bar>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .directive('originGamelibraryFilterBar', originGamelibraryFilterBar);
}());
