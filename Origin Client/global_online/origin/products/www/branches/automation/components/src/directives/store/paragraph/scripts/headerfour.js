
/** 
 * @file store/paragraph/scripts/headerfour.js
 */ 
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-paragraph-headerfour';

    function originStoreParagraphHeaderfour(UtilFactory) {
        function originStoreParagraphHeaderfourLink(scope) {
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
            template: '<h4 class="otktitle-4" ng-bind-html="::strings.description"></h4>',
            link: originStoreParagraphHeaderfourLink
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