/**
 * @file /infobubble.js
 */

(function() {
    'use strict';

    function originInfobubble() {
        return {
            restrict: 'E',
            replace: true,
            transclude: true,
            template:   '<div class="origin-infobubble">'+
                            '<div class="origin-infobubble-arrow origin-infobubble-arrow-up"></div>'+
                            '<div class="origin-infobubble-content" ng-transclude></div>'+
                        '</div>',
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originInfobubble
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * Info Bubble to show "More Info" about an item
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-infobubble />
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .directive('originInfobubble', originInfobubble);
}());