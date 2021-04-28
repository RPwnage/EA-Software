/**
 * @file gamelibrary/emptyonline.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-gamelibrary-empty-online';

    function originGamelibraryEmptyOnline(ComponentsConfigFactory, UtilFactory) {

        function originGamelibraryEmptyOnlineLink(scope, element, attributes) {
            scope.title = UtilFactory.getLocalizedStr(attributes.title, CONTEXT_NAMESPACE, 'title');
            scope.description = UtilFactory.getLocalizedStr(attributes.description, CONTEXT_NAMESPACE, 'description');
        }

        return {
            restrict: 'E',
            link: originGamelibraryEmptyOnlineLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/emptyonline.html'),
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameLibraryEmpty
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} title the empty library message title
     * @param {LocalizedText} description the empty library message body
     * @description
     *
     * * empty game library for online users
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-gamelibrary-empty-online
     *              title="Your game library is empty"
     *              description="Go online to purchase your first game or check out our selection of free games">
     *         </origin-gamelibrary-empty-online>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .directive('originGamelibraryEmptyOnline', originGamelibraryEmptyOnline);
}());