/**
 * @file dialog/content/playmultilaunch.js
 */

(function() {
    'use strict';

    function OriginDialogContentPlaymultilaunchCtrl($scope) {
        $scope.selectedOption = '0';
        $scope.dontShow = false;
    }

    function originDialogContentPlaymultilaunch(ComponentsConfigFactory) {
        function originDialogContentPlaymultilaunchLink(scope, elem, attrs, ctrl) {
            scope.options = JSON.parse(attrs.options);

            var promptCtrl = ctrl[0];
            promptCtrl.setContentDataFunc(function() {
                return {
                    dontShow: scope.dontShow,
                    option: scope.selectedOption
                };
            });
        }
        return {
            restrict: 'E',
            require: ['^originDialogContentPrompt', 'originDialogContentPlaymultilaunch'],
            scope: {
                dontshowtext: '@'
            },
            controller: 'OriginDialogContentPlaymultilaunchCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/playmultilaunch.html'),
            link: originDialogContentPlaymultilaunchLink
        };
    }

    angular.module('origin-components')
        .directive('originDialogContentPlaymultilaunch', originDialogContentPlaymultilaunch)
        .controller('OriginDialogContentPlaymultilaunchCtrl', OriginDialogContentPlaymultilaunchCtrl);

}());