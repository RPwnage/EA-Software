/**
 * @file format-date.js
 */
/* global moment */

(function() {
    'use strict';

    /**
     * @ngdoc filter
     * @name origin-components.filters:formatDate
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * Display date as a formatted string
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <span>{{ releaseDate | formatDate:'LL' }}</span>
     *     </file>
     * </example>
     *
     */
    function formatDateFilter() {
        return function (date, format) {
            return moment(date).format(format);
        };
    }

    angular.module('origin-components')
        .filter('formatDate', formatDateFilter);
}());
