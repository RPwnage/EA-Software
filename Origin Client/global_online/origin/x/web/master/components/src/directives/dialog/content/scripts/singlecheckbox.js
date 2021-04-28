/**
 * @file dialog/content/singlecheckbox.js
 */

(function() {
    'use strict';

    function OriginDialogContentSinglecheckboxCtrl($scope) {
        $scope.chkChecked = ($scope.checkeddefault === 'true' ? true : false);
    }

    /**
     * The dialog content to the play flow's mac Oig Settings prompt
     * @directive originDialogContentSinglecheckbox
     */
    function originDialogContentSinglecheckbox(ComponentsConfigFactory) {
        function originDialogContentSinglecheckboxLink(scope, elem, attrs, ctrl) {
            var promptCtrl = ctrl[0];
            promptCtrl.setContentDataFunc(function() {
                return {
                    chkChecked: scope.chkChecked
                };
            });
        }
        return {
            restrict: 'E',
            require: ['^originDialogContentPrompt', 'originDialogContentSinglecheckbox'],
            scope: {
                checkboxtext: '@',
                checkeddefault: '@'
            },
            controller: 'OriginDialogContentSinglecheckboxCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/singlecheckbox.html'),
            link: originDialogContentSinglecheckboxLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentSinglecheckbox
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * The dialog content to the play flow's mac Oig Settings prompt
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <a href='openOER' origin-dialog-content-singlecheckbox></a>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originDialogContentSinglecheckbox', originDialogContentSinglecheckbox)
        .controller('OriginDialogContentSinglecheckboxCtrl', OriginDialogContentSinglecheckboxCtrl);

}());