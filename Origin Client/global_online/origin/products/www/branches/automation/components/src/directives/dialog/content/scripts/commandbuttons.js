/**
 * @file dialog/content/commandbuttons.js
 */
(function() {
    'use strict';
    /**
     * Dynamic content for dialogs that have command buttons
     * @directive originDialogContentCommandbuttons
     */
    function originDialogContentCommandbuttons(ComponentsConfigFactory) {
        function originDialogContentCommandbuttonsLink(scope, elem, attrs, ctrl) {
            scope.commandButtons = JSON.parse(attrs.commandbuttons);
            var promptCtrl = ctrl[0];

            scope.click = function(option) {
                scope.selectedOption = option;
                if (option) {
                    promptCtrl.finish(true);
                }
            };
            
            promptCtrl.setContentDataFunc(function() {
                return {
                    selectedOption: scope.selectedOption
                };
            });
        }
        return {
            restrict: 'E',
            require: ['^originDialogContentPrompt'],
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/commandbuttons.html'),
            link: originDialogContentCommandbuttonsLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentCommandbuttons
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * Dynamic content for dialogs that have command buttons
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dialog-content-commandbuttons></origin-dialog-content-commandbuttons>
     *     </file>
     * </example>
     */

    // register to angular
    angular.module('origin-components')
        .directive('originDialogContentCommandbuttons', originDialogContentCommandbuttons);

}());