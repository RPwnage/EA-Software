
/** 
 * @file store/paragraph/scripts/headersix.js
 */ 
(function(){
    'use strict';

    /**
    * The directive
    */
    function originStoreParagraphHeadersix() {
        return {
            restrict: 'E',
            replace: true,
            scope: {
            	description: '@'
            },
            template: '<h6 class="otktitle-6">{{ description }}</h6>'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreParagraphHeadersix
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} description The text for this paragraph.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-paragraph-headersix 
     *     		description="Some text.">
     *     </origin-store-paragraph-headersix>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreParagraphHeadersix', originStoreParagraphHeadersix);
}());