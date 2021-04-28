/**
 * @file postrepeat.js
 */

(function() {
    'use strict';

    function originPostrepeat() {

        function originPostrepeatLink(scope, element, attrs) {
            if (scope.$last) {
                var eventName = attrs.originPostrepeat || 'last';
                scope.$emit('postrepeat:' + eventName);
            }
        }

        return {
            restrict: 'A',
            link: originPostrepeatLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originPostrepeat
     * @restrict A
     * @element ANY
     * @scope
     * @description
     *
     * Post repeat directive that emits a message when the last item in the ng-repeat has
     * completed rendering.
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <ul>
     *           <li ng-repeat="game in games" origin-postrepeat></li>
     *         </ul>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .directive('originPostrepeat', originPostrepeat);

}());