/**
 * @file filter/filterpill.js
 */
(function() {
    'use strict';

    function originFilterPill(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                label: '&',
                isActive: '&',
                onDismiss: '&'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('filter/views/filterpill.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originFilterPill
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} label filter label
     * @param {boolean} isActive flag indicating whether to display the filter or not
     * @param {function} onDismiss callback to call when the user closes the filter element
     * @description
     *
     * Dismissable active filter bubble
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-filterbar>
     *             <origin-filter-pill label="Filter by name" is-active="myFilter.active" on-dismiss="dismissFilter()"></origin-filter-bar-item>
     *         </origin-filterbar>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .directive('originFilterPill', originFilterPill);
}());
