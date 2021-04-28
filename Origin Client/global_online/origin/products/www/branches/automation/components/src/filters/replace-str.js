/**
 * @file replace-str.js
 */

(function() {
    'use strict';

    /**
     * @ngdoc filter
     * @name origin-components.filters:replaceStr
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * replace search in subject with given replace
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <span>{{ mystr | replace:'%replace%':'myreplace' }}</span>
     *     </file>
     * </example>
     *
     */
    function replaceFilter() {
        return function (subject, search, replace) {
            return subject.replace(search,replace);
        };
    }

    angular.module('origin-components')
        .filter('replaceStr', replaceFilter);
}());
