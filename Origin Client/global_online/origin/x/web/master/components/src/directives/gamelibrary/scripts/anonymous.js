/**
 * @file gamelibrary/anonymous.js
 */
(function() {
    'use strict';

    function originGamelibraryAnonymous(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/anonymous.html')
        };   
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameLibraryAnonymous
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} title the anonymouse user message title
     * @param {LocalizedText} description the anonymous user message description
     * @description
     *
     * game library anonymous container
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-gamelibrary-anonymous
     *              title="Games Library Unavailable"
     *              description="Log in to view your games">
     *         </origin-gamelibrary-anonymous>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .directive('originGamelibraryAnonymous', originGamelibraryAnonymous);
}());