/**
 * @file store/about/scripts/legacynotification.js
 */
(function () {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-about-legacynotification';

    function originStoreAboutLegacynotification(ComponentsConfigFactory, UtilFactory) {
        function originStoreAboutLegacynotificationLink(scope) {
            scope.showNotification = UtilFactory.isXPOrVista();
            scope.legacyMessage = UtilFactory.getLocalizedStr(scope.message, CONTEXT_NAMESPACE, 'message');
        }

        return {
            restrict: 'E',
            scope: {
                message: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/about/views/legacynotification.html'),
            link: originStoreAboutLegacynotificationLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAboutLegacynotification
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} message the message to show if user is using a legacy OS
     * 
     * @description show message if user is on legacy OS
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-about-legacynotification message=""></origin-store-about-legacynotification>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreAboutLegacynotification', originStoreAboutLegacynotification);
}());
