/**
 * @file dialog/content/downloadeulasummary.js
 */

(function() {
    'use strict';

    function OriginDialogContentDownloadeulasummaryCtrl($scope, DialogFactory, UtilFactory) {
        $scope.accepted = false;
        /**
         * Reaction from the 'eulaaccept' check box clicked. Enables the prompt's 'Yes' button.
         * @return {void}
         */
        $scope.onAcceptedChanged = function() {
            DialogFactory.update({acceptEnabled:$scope.accepted});
        };
        
        /**
         * Reaction from the user clicking on a eula link.
         * @return {void}
         */
        $scope.onEulaClick = function(index) {
            var selectedEula = $scope.eulas[index];
            // Need to reconvert this. Otherwise the ' will be messed up.
            selectedEula.description = UtilFactory.toAttributeFriendlyText(selectedEula.description);
            var content = DialogFactory.createDirective('origin-dialog-content-downloadeula', {
                agreechecktext: '',
                printtext: selectedEula.printtext,
                indextext: selectedEula.indextext,
                index: selectedEula.index,
                showaccept: false,
                eula: selectedEula
            });
            DialogFactory.openAlert({
                id: selectedEula.filename,
                title: selectedEula.name,
                description: '',
                rejectText: selectedEula.rejecttext,
                contentDirective: content
            });
        };
    }

    /**
     * Download Flow that shows a list of eulas
     * @directive originDialogContentDownloadeulasummary
     */
    function originDialogContentDownloadeulasummary(ComponentsConfigFactory) {
        function originDialogContentDownloadeulasummaryLink(scope, elem, attrs, ctrl) {
            scope.eulas = JSON.parse(attrs.eulas);
            var promptCtrl = ctrl[0];
            promptCtrl.setContentDataFunc(function() {
                return {
                    preventContinue: scope.accepted === false
                };
            });
        }
        return {
            restrict: 'E',
            require: ['^originDialogContentPrompt', 'originDialogContentDownloadeulasummary'],
            scope: {
                agreechecktext: '@'
            },
            controller: 'OriginDialogContentDownloadeulasummaryCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/downloadeulasummary.html'),
            link: originDialogContentDownloadeulasummaryLink
        };
    }


    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentDownloadeulasummary
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * Download Flow that shows a list of eulas
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dialog-content-downloadeulasummary></origin-dialog-content-downloadeulasummary>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originDialogContentDownloadeulasummary', originDialogContentDownloadeulasummary)
        .controller('OriginDialogContentDownloadeulasummaryCtrl', OriginDialogContentDownloadeulasummaryCtrl);

}());