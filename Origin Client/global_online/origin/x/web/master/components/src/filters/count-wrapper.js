/**
 * @file count-wrapper.js
 */

(function() {
    'use strict';

    /**
     * @ngdoc filter
     * @name origin-components.filters:countWrapper
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * Displays a string followed by a number in brackets. Zeroes are not displayed
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <span>{{ title | countWrapper:numberOfItems }}</span>
     *     </file>
     * </example>
     *
     */
    function countWrapperFilter() {
        return function (text, count) {
            if (count > 0) {
                return text + ' (' + count + ')';
            } else {
                return text;
            }
        };
    }

    angular.module('origin-components')
        .filter('countWrapper', countWrapperFilter);
}());
