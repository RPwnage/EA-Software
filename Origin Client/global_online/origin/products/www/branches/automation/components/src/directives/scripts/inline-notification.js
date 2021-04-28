/**
 * @file /scripts/inline-notification.js
 */
(function () {
    'use strict';

    var TypeEnumeration = {
        "standard": "standard",
        "warning": "warning",
        "error": "error"
    };
    var CONTEXT_NAMESPACE = 'origin-inline-notification';
    var TYPE_TO_CLASS = {
        standard: 'otkicon-star',
        warning: 'otkicon-warning',
        error: 'otkicon-warning'
    };

    function originInlineNotification(ComponentsConfigFactory, UtilFactory) {
        function originInlineNotificationLink(scope) {
            scope.type = scope.type || TypeEnumeration.standard;
            scope.TypeEnumeration = TypeEnumeration;
            scope.message = UtilFactory.getLocalizedStr(scope.message, CONTEXT_NAMESPACE, 'message');
            scope.typeClass = TYPE_TO_CLASS[scope.type];
        }

        return {
            restrict: 'E',
            scope: {
                message: '@',
                type: '@'
            },
            link: originInlineNotificationLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('views/inline-notification.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originInlineNotification
     * @restrict
     * @element ANY
     * @scope
     *
     * @param {LocalizedString} message message of the inline notification
     * @param {TypeEnumeration} type type of the inline notification. Optional. Standard, Warning or Error. Defaults to Standard.
     *
     * @description
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-inline-notification></origin-inline-notification>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originInlineNotification', originInlineNotification);
}());
