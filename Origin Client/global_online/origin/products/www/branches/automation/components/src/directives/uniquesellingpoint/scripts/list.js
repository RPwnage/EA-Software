
/** 
 * @file /Uniquesellingpoint/scripts/list.js
 */ 
(function(){
    'use strict';

    function originUniquesellingpointList(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {},
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('uniquesellingpoint/views/list.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originUniquesellingpointList
     * @restrict E
     * @element ANY
     * @scope
     * @description A wrapper directive for the Unique Selling Point List component. Add unique selling point items to this list
     * to populate. 
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-uniquesellingpoint-list />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originUniquesellingpointList', originUniquesellingpointList);
}()); 
