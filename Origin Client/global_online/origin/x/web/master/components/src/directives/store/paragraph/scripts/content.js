/**
 * @file store/paragraph/scripts/content.js
 */
(function(){
    'use strict';

    function originStoreParagraphContent() {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                description: '@'
            },
            template: '<p class="otkc">{{ description }}</p>'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreParagraphContent
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
     *     <origin-store-paragraph-content
     *          description="Some text.">
     *     </origin-store-paragraph-content>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreParagraphContent', originStoreParagraphContent);
}());