
/**
 * @file /scripts/listitem.js
 */
(function(){
    'use strict';
    function originListitem() {
        return {
            restrict: 'E',
            template: "<li ng-transclude></li>",
            transclude: true,
            replace: true
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originListitem
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-listitem></origin-listitem>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originListitem', originListitem);
}());
