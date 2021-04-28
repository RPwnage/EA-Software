/**
 * @file dialog/content/downloadinfo.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-dialog-content-downloadinfo';


    function OriginDialogContentDownloadinfoCtrl($scope, AppCommFactory, UtilFactory) {
        $scope.extraContentText = UtilFactory.getLocalizedStr($scope.extraContentText, CONTEXT_NAMESPACE, 'extracontentstr');
        /**
         * Update the content
         * @return {void}
         */
        function updatePriv(conf) {
            if(conf.contentType !== 'origin-dialog-content-downloadinfo') {
                return;
            }
            $scope.insufficientspace = conf.content.insufficientspace;
            $scope.insufficientspacetext = conf.content.insufficientspacetext;
            $scope.changeinstallationlocationtext = conf.content.changeinstallationlocationtext;
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

        function createHyperlink(text, ngClick) {
            return '<a class="otka" ng-click="' + ngClick + '">' + text + '</a>';
        }

        var localizer = UtilFactory.getLocalizedStrGenerator(CONTEXT_NAMESPACE);

        if ($scope.insufficientspace === 'true' || $scope.insufficientspace === true) {
            $scope.insufficientspacetext = localizer($scope.insufficientspacetext, 'insufficientspacetext', {
                '%changeinstallationlocationtext%': createHyperlink($scope.changeinstallationlocationtext, 'changeInstallationLocation()')
            });
        }

        $scope.changeInstallationLocation = function() {
            Origin.client.installDirectory.chooseDownloadInPlaceLocation();
        };

        // subscribe to events
        AppCommFactory.events.on('dialog:update', updatePriv);
        $scope.$on('$destroy', onDestroy);
    }

    /**
     * Download Flow info content. Shows install space, disc space, etc.
     * @directive originDialogContentDownloadinfo
     */
    function originDialogContentDownloadinfo(ComponentsConfigFactory, $compile) {
        function originDialogContentDownloadinfoLink(scope, elem, attrs, ctrl) {
            scope.extracheckbuttons = JSON.parse(attrs.extracheckbuttons || '{}');
            scope.extracontent = JSON.parse(attrs.extracontent || '{}');

            if (scope.insufficientspace === 'true' || scope.insufficientspace === true) {
                elem.find('.origin-cldialog-downloadinfo-warning-text').append($compile('<span>'+scope.insufficientspacetext+'</span>')(scope));
            }

            var promptCtrl = ctrl[0];
            promptCtrl.setContentDataFunc(function() {
                var checkButtonResult = [], extraContentResult = [];
                for (var iChk in scope.extracheckbuttons) {
                    checkButtonResult.push({
                        id: scope.extracheckbuttons[iChk].id,
                        checked: scope.extracheckbuttons[iChk].checked
                    });
                }
                for (var iCont in scope.extracontent) {
                    if(scope.extracontent[iCont].checked) {
                        extraContentResult.push(scope.extracontent[iCont].productId);
                    }
                }
                return {
                    extracheckbuttons: checkButtonResult,
                    extracontent: extraContentResult
                };
            });
        }
        return {
            restrict: 'E',
            require: ['^originDialogContentPrompt', 'originDialogContentDownloadinfo'],
            scope: {
                insufficientspace: '@',
                insufficientspacetext: '@',
                changeinstallationlocationtext: '@',
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
     * @param {LocalizedString} extracontentstr *Update in Defaults
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