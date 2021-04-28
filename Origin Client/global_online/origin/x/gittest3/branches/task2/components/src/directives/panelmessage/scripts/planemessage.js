/**
 * @file planemessage/planemessage.js
 */
(function() {
    'use strict';

    function originPlanemessage(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                title: '@',
                description: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('panelmessage/views/panelmessage.html'),
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originPlanemessage
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} title message title
     * @param {string} description message body
     * @description
     *
     * plane message with the title and body (description)
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-planemessage
     *              title="Some bold title"
     *              description="something in fine print">
     *         </origin-planemessage>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .directive('originPlanemessage', originPlanemessage);
}());