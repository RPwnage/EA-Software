/**
 * @file dialog/content/downloadeula.js
 */

(function() {
    'use strict';

    function OriginDialogContentDownloadeulaCtrl($scope, DialogFactory) {
        $scope.accepted = false;
        /**
         * Reaction from the 'eulaaccept' check box clicked. Enables the prompt's 'Yes' button.
         * @return {void}
         */
        $scope.onAcceptedChanged = function() {
            DialogFactory.update({acceptEnabled:$scope.accepted});
        };
    }

    /**
     * Download Flow single eula content
     * @directive originDialogContentDownloadeula
     */
    function originDialogContentDownloadeula(ComponentsConfigFactory) {
        function originDialogContentDownloadeulaLink(scope, elem, attrs, ctrl) {
            scope.eula = JSON.parse(attrs.eula);
            var promptCtrl = ctrl[0];
            promptCtrl.setContentDataFunc(function() {
                return {
                    preventContinue: scope.accepted === false
                };
            });
        }
        return {
            restrict: 'E',
            require: ['^originDialogContentPrompt', 'originDialogContentDownloadeula'],
            scope: {
                agreechecktext: '@',
                showaccept: '@',
                printtext: '@',
                indextext: '@',
                index: '@'
            },
            controller: 'OriginDialogContentDownloadeulaCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/downloadeula.html'),
            link: originDialogContentDownloadeulaLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentDownloadeula
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * Download Flow single eula content
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dialog-content-downloadeula></origin-dialog-content-downloadeula>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originDialogContentDownloadeula', originDialogContentDownloadeula)
        .controller('OriginDialogContentDownloadeulaCtrl', OriginDialogContentDownloadeulaCtrl);

}());