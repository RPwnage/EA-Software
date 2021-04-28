/**
 * @file dialog/content/playmultilaunch.js
 */

(function() {
    'use strict';
    /* jshint ignore:start */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */
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

     /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentPlaymultilaunch
     * @restrict E
     * @element ANY
     * @scope
     * @param {BooleanEnumeration} dontshowtext - achievements or points
     * @description
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dialog-content-playmultilaunch dontshowtext="true"></origin-dialog-content-playmultilaunch>
     *     </file>
     * </example>
     */

    angular.module('origin-components')
        .directive('originDialogContentPlaymultilaunch', originDialogContentPlaymultilaunch)
        .controller('OriginDialogContentPlaymultilaunchCtrl', OriginDialogContentPlaymultilaunchCtrl);

}());