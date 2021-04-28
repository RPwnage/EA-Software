/**
 * @file dialog/content/youneedorigin.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-dialog-content-youneedorigin';
    var BETA_FIXTURE_KEY = 'beta-message';
    var BETA_TIMER_FIXTURE_KEY = 'beta-timer';
    var BETA_MESSAGE_MISS_KEY = 'miss' + BETA_FIXTURE_KEY;
    var BETA_MESSAGE_TIMER_MISS_KEY = 'miss' + BETA_TIMER_FIXTURE_KEY;

    function originDialogContentYouneedorigin(ComponentsConfigFactory, DialogFactory, ProductFactory, UrlHelper, UtilFactory, moment) {

        function originDialogContentYouneedoriginLink(scope) {
            scope.strings = {
                title: UtilFactory.getLocalizedStr(scope.titleStr, CONTEXT_NAMESPACE, 'title-str'),
                downloadAndInstall: UtilFactory.getLocalizedStr(scope.downloadAndInstallStr, CONTEXT_NAMESPACE, 'download-and-install-str'),
                launchAndSignIn: UtilFactory.getLocalizedStr(scope.launchAndSignInStr, CONTEXT_NAMESPACE, 'launch-and-sign-in-str'),
                downloadAndPlay: UtilFactory.getLocalizedStr(scope.downloadAndPlayStr, CONTEXT_NAMESPACE, 'download-and-play-str'),
                downloadOrigin: UtilFactory.getLocalizedStr(scope.downloadOriginStr, CONTEXT_NAMESPACE, 'download-origin-str'),
                continueBrowsing: UtilFactory.getLocalizedStr(scope.continueBrowsingStr, CONTEXT_NAMESPACE, 'continue-browsing-str'),
                betaMessage: UtilFactory.getLocalizedStr(scope.betaMessage, CONTEXT_NAMESPACE, BETA_FIXTURE_KEY),
                betaMessageTimer: UtilFactory.getLocalizedStr(scope.betaMessage, CONTEXT_NAMESPACE, BETA_TIMER_FIXTURE_KEY)
            };

            //For beta message
            if (scope.strings.betaMessage.indexOf(BETA_MESSAGE_MISS_KEY) > -1){
                scope.strings.betaMessage = '';
            }else if (scope.strings.betaMessageTimer.indexOf(BETA_MESSAGE_TIMER_MISS_KEY) === -1){
                var betaMessageTimer = moment(scope.strings.betaMessageTimer);
                var now = moment();
                if (now.isAfter(betaMessageTimer)){
                    scope.strings.betaMessage = '';
                }
            }

            scope.dismissModal = function() {
                DialogFactory.close(scope.id);
            };

            scope.model = {};
            var removeWatcher = scope.$watch('offerId', function (newValue) {
                if (newValue) {
                    removeWatcher();
                    ProductFactory.get(newValue).attachToScope(scope, 'model');
                }
            });

            scope.downloadLink = UrlHelper.getClientDownloadLink();
            scope.platform = Origin.utils.os();
        }

        return {
            restrict: 'E',
            scope: {
                offerId: '@',
                id: '@',
                titleStr: '@',
                launchAndSignInStr: '@',
                downloadAndInstallStr: '@',
                downloadAndPlayStr: '@',
                downloadOriginStr: '@',
                continueBrowsingStr: '@',
                betaMessageTimer: '@',
                betaMessage: '@'
            },
            link: originDialogContentYouneedoriginLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/youneedorigin.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentYouneedorigin
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString|OCD} title-str The "You Need Origin" title text
     * @param {LocalizedString|OCD} download-and-install-str "Download and Install Origin" text
     * @param {LocalizedString|OCD} launch-and-sign-in-str "Launch Origin and Sign In" text
     * @param {LocalizedString|OCD} download-and-play-str "Download and Play" text
     * @param {LocalizedString|OCD} download-origin-str "Download Origin" button text
     * @param {LocalizedString|OCD} continue-browsing-str "Continue Browsing" button text
     *
     * @param {LocalizedString} beta-timer timer for beta message. E.G., 2016-09-07T:00:00:00. Beta message will show until this time is passed.
     * @param {LocalizedString} beta-message client beta message. Don't merchandise if you don't want it to show.
     *
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
        .directive('originDialogContentYouneedorigin', originDialogContentYouneedorigin);

}());