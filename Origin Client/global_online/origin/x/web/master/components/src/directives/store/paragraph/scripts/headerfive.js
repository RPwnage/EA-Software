
/** 
 * @file store/paragraph/scripts/headerfive.js
 */ 
(function(){
    'use strict';

    /**
    * The directive
    */
    function originStoreParagraphHeaderfive() {
        return {
            restrict: 'E',
            replace: true,
            scope: {
            	description: '@'
            },
            template: '<h5 class="otktitle-5">{{ description }}</h5>'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreParagraphHeaderfive
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
     *     <origin-store-paragraph-headerfive 
     *     		description="Some text.">
     *     </origin-store-paragraph-headerfive>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreParagraphHeaderfive', originStoreParagraphHeaderfive);
}());