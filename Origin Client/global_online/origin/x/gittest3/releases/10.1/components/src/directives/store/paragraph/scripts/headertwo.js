
/** 
 * @file store/paragraph/scripts/headertwo.js
 */ 
(function(){
    'use strict';

    /**
    * The directive
    */
    function originStoreParagraphHeadertwo() {
        return {
            restrict: 'E',
            replace: true,
            scope: {
            	description: '@'
            },
            template: '<h2 class="otktitle-2">{{ description }}</h2>'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreParagraphHeadertwo
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
     *     <origin-store-paragraph-headertwo 
     *     		description="Some text.">
     *     </origin-store-paragraph-headertwo>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreParagraphHeadertwo', originStoreParagraphHeadertwo);
}());