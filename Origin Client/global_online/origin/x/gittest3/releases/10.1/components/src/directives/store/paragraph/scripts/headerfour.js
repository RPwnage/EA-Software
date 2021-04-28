
/** 
 * @file store/paragraph/scripts/headerfour.js
 */ 
(function(){
    'use strict';

    /**
    * The directive
    */
    function originStoreParagraphHeaderfour() {
        return {
            restrict: 'E',
            replace: true,
            scope: {
            	description: '@'
            },
            template: '<h4 class="otktitle-4">{{ description }}</h4>'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreParagraphHeaderfour
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
     *     <origin-store-paragraph-headerfour 
     *     		description="Some text.">
     *     </origin-store-paragraph-headerfour>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreParagraphHeaderfour', originStoreParagraphHeaderfour);
}());