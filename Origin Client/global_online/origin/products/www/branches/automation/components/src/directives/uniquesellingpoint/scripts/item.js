
/** 
 * @file /uniquesellingpoint/scripts/item.js
 */ 
(function(){
    'use strict';

    function originUniquesellingpointItem(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                titleStr: '@',
                content: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('/uniquesellingpoint/views/item.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originUniquesellingpointItem
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedText} title-str The heading for the USP item
     * @param {LocalizedText} content The encoded html content of the USP item
     * @description A unique selling point list item with title and encoded html for content. This readers by default 
     * with a green checkbox bullet icon.  
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-uniquesellingpoint-item />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originUniquesellingpointItem', originUniquesellingpointItem);
}()); 
