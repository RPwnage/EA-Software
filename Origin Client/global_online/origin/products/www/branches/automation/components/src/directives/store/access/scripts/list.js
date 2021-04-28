
/** 
 * @file store/access/scripts/list.js
 */ 
(function(){
    'use strict';

    function originStoreAccessList() {
        return {
            restrict: 'E',
            transclude: true,
            replace: true,
            scope: {},
            template: '<ul class="origin-store-access-list otktitle-3" ng-transclude></ul>'
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAccessList
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-access-list />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreAccessList', originStoreAccessList);
}()); 
