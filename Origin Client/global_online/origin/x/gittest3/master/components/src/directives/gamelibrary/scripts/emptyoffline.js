/**
 * @file gamelibrary/emptyoffline.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-gamelibrary-empty-offline';

    function originGamelibraryEmptyOffline(ComponentsConfigFactory, UtilFactory) {

        function originGamelibraryEmptyOfflineLink(scope, element, attributes) {
            scope.title = UtilFactory.getLocalizedStr(attributes.title, CONTEXT_NAMESPACE, 'title');
            scope.description = UtilFactory.getLocalizedStr(attributes.description, CONTEXT_NAMESPACE, 'description');
        }

        return {
            restrict: 'E',
            link: originGamelibraryEmptyOfflineLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/emptyoffline.html'),
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
     * empty game library for offline users
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-gamelibrary-empty-offline
     *              title="Your game library is empty"
     *              description="Go online to purchase your first game or check out our selection of free games">
     *         </origin-gamelibrary-empty-offline>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .directive('originGamelibraryEmptyOffline', originGamelibraryEmptyOffline);
}());