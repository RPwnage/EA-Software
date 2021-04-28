/**
 * @file dialog/content/downloadinfo.js
 */

(function() {
    'use strict';

    function OriginDialogContentDownloadinfoCtrl($scope, AppCommFactory) {
        /**
         * Update the content
         * @return {void}
         */
        function updatePriv(conf) {
            if(conf.contentType !== 'origin-dialog-content-downloadinfo') {
                return;
            }
            $scope.insufficientspacetext = conf.content.insufficientspacetext;
            $scope.size1text = conf.content.size1text;
            $scope.size2text = conf.content.size2text;
            $scope.size1 = conf.content.size1;
            $scope.size2 = conf.content.size2;
            $scope.$digest();
        }

        /**
        * Clean up after yoself
        * @method onDestroy
        * @return {void}
        */
        function onDestroy() {
            AppCommFactory.events.off('dialog:update', updatePriv);
        }

        // subscribe to events
        AppCommFactory.events.on('dialog:update', updatePriv);
        $scope.$on('$destroy', onDestroy);
    }

    /**
     * Download Flow info content. Shows install space, disc space, etc.
     * @directive originDialogContentDownloadinfo
     */
    function originDialogContentDownloadinfo(ComponentsConfigFactory) {
        function originDialogContentDownloadinfoLink(scope, elem, attrs, ctrl) {
            scope.extracheckbuttons = JSON.parse(attrs.extracheckbuttons);
            var promptCtrl = ctrl[0];
            promptCtrl.setContentDataFunc(function() {
                var checkButtonResult = [];
                for (var i in scope.extracheckbuttons) {
                    checkButtonResult.push({
                        id: scope.extracheckbuttons[i].id,
                        checked: scope.extracheckbuttons[i].checked
                    });
                }
                return {
                    extracheckbuttons: checkButtonResult
                };
            });
        }
        return {
            restrict: 'E',
            require: ['^originDialogContentPrompt', 'originDialogContentDownloadinfo'],
            scope: {
                insufficientspacetext: '@',
                size1text: '@',
                size2text: '@',
                size1: '@',
                size2: '@'
            },
            controller: 'OriginDialogContentDownloadinfoCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/downloadinfo.html'),
            link: originDialogContentDownloadinfoLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentDownloadinfo
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * Download Flow info content. Shows install space, disc space, etc.
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dialog-content-downloadinfo></origin-dialog-content-downloadinfo>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originDialogContentDownloadinfo', originDialogContentDownloadinfo)
        .controller('OriginDialogContentDownloadinfoCtrl', OriginDialogContentDownloadinfoCtrl);

}());