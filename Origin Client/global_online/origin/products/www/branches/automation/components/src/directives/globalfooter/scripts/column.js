
/** 
 * @file globalfooter/scripts/column.js
 */ 
(function(){
    'use strict';
    function originGlobalfooterColumn(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            replace: true,
            scope: {
                columnTitle: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('globalfooter/views/column.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originGlobalfooterColumn
     * @restrict E
     * @element ANY
     * @scope
     * @description This directive provisions a column of links in the global footer.
     * @param {LocalizedText} column-title The title of the footer column ex. '' 
     *
     *
     * @example
     * <example module="origin-globalfooter-column">
     *     <file name="index.html">
     *         <origin-globalfooter-column column-title="Electronic Arts">
     *             <origin-globalfooter-link link-text="Legal Notices" link-url="#"/>
     *             <origin-globalfooter-link link-text="Terms of Sale" link-url="#"/>
     *             <origin-globalfooter-link link-text="Terms of Service" link-url="#"/>
     *         </origin-globalfooter-column>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originGlobalfooterColumn', originGlobalfooterColumn);
}()); 
