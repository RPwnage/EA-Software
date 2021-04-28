
/** 
 * @file store/paragraph/scripts/headersix.js
 */ 
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-paragraph-headersix';

    function originStoreParagraphHeadersix(UtilFactory) {
        function originStoreParagraphHeadersixLink(scope) {
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
            template: '<h6 class="otktitle-6" ng-bind-html="::strings.description"></h6>',
            link: originStoreParagraphHeadersixLink
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