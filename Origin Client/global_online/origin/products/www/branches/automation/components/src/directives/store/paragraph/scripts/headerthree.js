
/** 
 * @file store/paragraph/scripts/headerthree.js
 */ 
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-paragraph-headerthree';

    function originStoreParagraphHeaderthree(UtilFactory) {
        function originStoreParagraphHeaderthreeLink(scope) {
            scope.strings = {
                description: UtilFactory.getLocalizedStr(scope.description, CONTEXT_NAMESPACE, 'description')
            };
        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
            	description: '@'
            },
            template: '<h3 class="otktitle-3" ng-bind-html="::strings.description"></h3>',
            link: originStoreParagraphHeaderthreeLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreParagraphHeaderthree
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
     *     <origin-store-paragraph-headerthree 
     *     		description="Some text.">
     *     </origin-store-paragraph-headerthree>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreParagraphHeaderthree', originStoreParagraphHeaderthree);
}());