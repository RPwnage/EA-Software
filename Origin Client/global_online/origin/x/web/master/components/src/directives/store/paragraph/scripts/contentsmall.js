
/** 
 * @file store/paragraph/scripts/contentsmall.js
 */ 
(function(){
    'use strict';

    /**
    * The directive
    */
    function originStoreParagraphContentsmall() {
        return {
            restrict: 'E',
            replace: true,
            scope: {
            	description: '@'
            },
            template: '<p class="otkc otkc-small">{{ description }}</p>'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreParagraphContentsmall
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
     *     <origin-store-paragraph-contentsmall
     *     		description="Some text.">
     *     </origin-store-paragraph-contentsmall>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreParagraphContentsmall', originStoreParagraphContentsmall);
}());