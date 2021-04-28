/**
 * @file pad-zeroes.js
 */

(function() {
    'use strict';

    /**
     * @ngdoc filter
     * @name origin-components.filters:padZeroes
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * Add leading zeroes to an integer number for presentation
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <span>{{ integerNumber | padZeroes:2 }}</span>
     *     </file>
     * </example>
     *
     */
    function padZeroes() {
        /**
         * Given an integer, pad it with zeroes. Useful for formatting countdown timers
         * @param  {String|Number} text an integer as a string
         * @param  {Number} count The total fixed length of the resulting string
         * @return {string} the padded string eg "02" rather than 2
         */
        return function (text, count) {
            return ('' + 1e8 + text).slice(-count);
        };
    }

    angular.module('origin-components')
        .filter('padZeroes', padZeroes);
}());
