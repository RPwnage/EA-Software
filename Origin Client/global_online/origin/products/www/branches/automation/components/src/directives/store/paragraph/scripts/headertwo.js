
/** 
 * @file store/paragraph/scripts/headertwo.js
 */ 
(function(){
    'use strict';

    /* jshint ignore:start */
        /**
         * Sets the border of this module.
         * @readonly
         * @enum {boolean}
         */
        var BorderEnumeration = {
            "True": "true",
            "False": "false"
        };

    /* jshint ignore:end */

    var CONTEXT_NAMESPACE = 'origin-store-paragraph-headertwo';

    function originStoreParagraphHeadertwo(UtilFactory) {
        function originStoreParagraphHeadertwoLink(scope) {
            scope.borderClass = scope.border === 'true' ? ' origin-store-paragraph-headertwo-border' : '';

            scope.strings = {
                description: UtilFactory.getLocalizedStr(scope.description, CONTEXT_NAMESPACE, 'description')
            };
        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
            	description: '@',
                border: '@'
            },
            template: '<h2 class="otktitle-2{{ ::borderClass }}" ng-bind-html="::strings.description"></h2>',
            link: originStoreParagraphHeadertwoLink
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
     * @param {BorderEnumeration} border Add an underline to this header module
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-paragraph-headertwo 
     *     		description="Some text."
     *          border="true">
     *     </origin-store-paragraph-headertwo>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreParagraphHeadertwo', originStoreParagraphHeadertwo);
}());