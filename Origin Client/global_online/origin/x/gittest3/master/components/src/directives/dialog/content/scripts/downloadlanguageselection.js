/**
 * @file dialog/content/downloadlanguageselection.js
 */

(function() {
    'use strict';

    function OriginDialogContentDownloadlanguageselectionCtrl($scope) {
        $scope.locales = [];
    }

    /**
     * Download language selection content
     * @directive originDialogContentDownloadlanguageselection
     */
    function originDialogContentDownloadlanguageselection(ComponentsConfigFactory) {
        function originDialogContentDownloadlanguageselectionLink(scope, elem, attrs, ctrl) {
            scope.locales = JSON.parse(attrs.locales);
            scope.currentLocale = scope.locales[scope.currentindex];

            var promptCtrl = ctrl[0];
            promptCtrl.setContentDataFunc(function() {
                return {
                    localeId: scope.currentLocale.id
                };
            });
        }

        return {
            restrict: 'E',
            require: ['^originDialogContentPrompt', 'originDialogContentDownloadlanguageselection'],
            scope: {
                currentindex: '@'
            },
            controller: 'OriginDialogContentDownloadlanguageselectionCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/downloadlanguageselection.html'),
            link: originDialogContentDownloadlanguageselectionLink
        };
    }

	/**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentDownloadlanguageselection
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * Download Flow language select content
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dialog-content-downloadlanguageselection></origin-dialog-content-downloadlanguageselection>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originDialogContentDownloadlanguageselection', originDialogContentDownloadlanguageselection)
        .controller('OriginDialogContentDownloadlanguageselectionCtrl', OriginDialogContentDownloadlanguageselectionCtrl);

}());