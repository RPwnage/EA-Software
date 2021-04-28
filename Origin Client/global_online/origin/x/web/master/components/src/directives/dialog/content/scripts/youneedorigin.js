/**
 * @file dialog/content/youneedorigin.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-dialog-content-youneedorigin';

    function originDialogContentYouneedoriginCtrl($scope, $attrs, $sce, AppCommFactory, ComponentsUrlsFactory, DialogFactory, ObjectHelperFactory, PdpUrlFactory, ProductFactory, UtilFactory) {
        $scope.strings = {
            title: UtilFactory.getLocalizedStr($attrs.titleStr, CONTEXT_NAMESPACE, 'title'),
            downloadAndInstall: UtilFactory.getLocalizedStr($attrs.downloadAndInstallStr, CONTEXT_NAMESPACE, 'downloadAndInstall'),
            launchAndSignIn: UtilFactory.getLocalizedStr($attrs.launchAndSignInStr, CONTEXT_NAMESPACE, 'launchAndSignIn'),
            downloadAndPlay: UtilFactory.getLocalizedStr($attrs.downloadAndPlayStr, CONTEXT_NAMESPACE, 'downloadAndPlay'),
            downloadOrigin: UtilFactory.getLocalizedStr($attrs.downloadOriginStr, CONTEXT_NAMESPACE, 'downloadOrigin'),
            continueBrowsing: UtilFactory.getLocalizedStr($attrs.continueBrowsingStr, CONTEXT_NAMESPACE, 'continueBrowsing')
        };

        $scope.dismissModal = function() {
            DialogFactory.close();
        };

        $scope.model = {};
        var removeWatcher = $scope.$watch('offerId', function (newValue) {
            if (newValue) {
                ProductFactory.get(newValue).attachToScope($scope, 'model');
                removeWatcher();
            }
        });
    }

    function originDialogContentYouneedorigin(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerId: '@'
            },
            controller: 'originDialogContentYouneedoriginCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/youneedorigin.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentYouneedorigin
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString|OCD} titleStr The "You Need Origin" title text
     * @param {LocalizedString|OCD} downloadAndInstallStr "Download and Install Origin" text
     * @param {LocalizedString|OCD} launchAndSignInStr "Launch Origin and Sign In" text
     * @param {LocalizedString|OCD} downloadAndPlayStr "Download and Play" text
     * @param {LocalizedString|OCD} downloadOriginStr "Download Origin" button text
     * @param {LocalizedString|OCD} continueBrowsingStr "Continue Browsing" button text
     * @description
     *
     * "You Need Origin" dialogue which prompts user to download Origin client if needed
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dialog-content-youneedorigin>
     *         </origin-dialog-content-youneedorigin>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('originDialogContentYouneedoriginCtrl', originDialogContentYouneedoriginCtrl)
        .directive('originDialogContentYouneedorigin', originDialogContentYouneedorigin);

}());