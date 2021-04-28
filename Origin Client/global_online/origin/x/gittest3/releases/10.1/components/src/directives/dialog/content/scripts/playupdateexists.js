/**
 * @file dialog/content/playupdateexists.js
 */
(function() {
    'use strict';
    
    function originDialogContentPlayupdateexistsCtrl($scope) {
        $scope.chkChecked = ($scope.checkeddefault === 'true' ? true : false);
    }
    
    /**
     * Dynamic content for dialogs that have command buttons
     * @directive originDialogContentCommandbuttons
     */
    function originDialogContentPlayupdateexists(ComponentsConfigFactory) {
        function originDialogContentPlayupdateexistsLink(scope, elem, attrs, ctrl) {
            scope.commandbuttons = JSON.parse(attrs.commandbuttons);
            var promptCtrl = ctrl[0];
            elem.bind('click', function () {
                if (scope.selectedOption) {
                    promptCtrl.finish(true);
                }
            });
            promptCtrl.setContentDataFunc(function() {
                return {
                    selectedOption: scope.selectedOption,
                    chkChecked: scope.chkChecked
                };
            });
        }
        return {
            restrict: 'E',
            require: ['^originDialogContentPrompt'],
            scope: {
                checkboxtext: '@',
                checkeddefault: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/playupdateexists.html'),
            controller: 'originDialogContentPlayupdateexistsCtrl',
            link: originDialogContentPlayupdateexistsLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentPlayupdateexists
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
     *         <origin-dialog-content-playupdateexists></origin-dialog-content-playupdateexists>
     *     </file>
     * </example>
     */

    // register to angular
    angular.module('origin-components')
        .directive('originDialogContentPlayupdateexists', originDialogContentPlayupdateexists)
        .controller('originDialogContentPlayupdateexistsCtrl', originDialogContentPlayupdateexistsCtrl);

}());