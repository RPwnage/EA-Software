/**
 * @file panelmessage/panelmessage.js
 */
(function() {
    'use strict';

    function originPanelmessage(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                titleStr: '@',
                description: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('panelmessage/views/panelmessage.html'),
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originPanelmessage
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} title-str message title
     * @param {LocalizedText} description message body
     * @description
     *
     * plane message with the title and body (description)
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-panelmessage
     *              title="Some bold title"
     *              description="something in fine print">
     *         </origin-panelmessage>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .directive('originPanelmessage', originPanelmessage);
}());