/**
 * @file dialog/content/downloadthirdparty.js
 */
(function() {
    'use strict';

    /**
     * Download third party content
     * @directive originDialogContentDownloadthirdparty
     */
    function originDialogContentDownloadthirdparty(ComponentsConfigFactory) {
        function originDialogContentDownloadthirdpartyLink(scope, elem, attrs, ctrl) {
            var promptCtrl = ctrl[0];
            elem.bind('click', function () {
                if (scope.selectedOption) {
                    promptCtrl.finish(true);
                }
            });
            promptCtrl.setContentDataFunc(function() {
                return {
                    selectedOption: scope.selectedOption
                };
            });
        }
        return {
            restrict: 'E',
            require: ['^originDialogContentPrompt'],
            scope: {
                step1title: '@',
                step1desc: '@',
                cdkeytitle: '@',
                cdkey: '@',
                step2title: '@',
                step2desc: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/downloadthirdparty.html'),
            link: originDialogContentDownloadthirdpartyLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentDownloadthirdparty
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * Download Flow third party dialog content
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dialog-content-downloadthirdparty></origin-dialog-content-downloadthirdparty>
     *     </file>
     * </example>
     */
    // register to angular
    angular.module('origin-components')
        .directive('originDialogContentDownloadthirdparty', originDialogContentDownloadthirdparty);

}());