/**
 * @file settings.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-settings';

    function OriginSettingsCtrl($scope, $sce, $stateParams, $state, $rootScope, UtilFactory, ComponentsConfigHelper) {

        $scope.inClient = Origin.client.isEmbeddedBrowser();

        $scope.changeSavedStr = UtilFactory.getLocalizedStr($scope.changeSavedStr, CONTEXT_NAMESPACE, 'change-saved');
        $scope.footerStr = $sce.trustAsHtml(UtilFactory.getLocalizedStr($scope.footer, CONTEXT_NAMESPACE, 'footer'));
        $scope.footerUrlStr = ComponentsConfigHelper.getHelpUrl(Origin.locale.locale(), Origin.locale.languageCode());

        $scope.tabNames = [];
        $scope.tabNames[0] = UtilFactory.getLocalizedStr($scope.applicationstr, CONTEXT_NAMESPACE, "tab-application");
        $scope.tabNames[1] = UtilFactory.getLocalizedStr($scope.diagnosticsstr, CONTEXT_NAMESPACE, "tab-diagnostics");
        $scope.tabNames[2] = UtilFactory.getLocalizedStr($scope.installsstr, CONTEXT_NAMESPACE, "tab-installs");
        $scope.tabNames[3] = UtilFactory.getLocalizedStr($scope.notificationsstr, CONTEXT_NAMESPACE, "tab-notifications");
        $scope.tabNames[4] = UtilFactory.getLocalizedStr($scope.oigstr, CONTEXT_NAMESPACE, "tab-oig");
        $scope.tabNames[5] = UtilFactory.getLocalizedStr($scope.voicestr, CONTEXT_NAMESPACE, "tab-voice");

        var startingIndex = 0,
            endingIndex = 6,
            pageNames = [
                "application",
                "diagnostics",
                "installs",
                "notifications",
                "oig",
                "voice"
            ];

        $scope.pages = {
            application: pageNames[0],
            diagnostics: pageNames[1],
            installs: pageNames[2],
            notifications: pageNames[3],
            oig: pageNames[4],
            payment: "payment",
            subscriptions: "subscriptions",
            voice: pageNames[5]
        };

        $scope.items = [];
        $scope.activeItemIndex = 0;
        $scope.isUnderAge = Origin.user.underAge();

        // for now assume that if we have extra parameters on the window it's an OIG window
        if (!!ComponentsConfigHelper.getOIGContextConfig()){
            startingIndex = 3;  // if in OIG, only show last 3 settings pages
        }
        if (!Origin.voice.supported() || $scope.isUnderAge){
            endingIndex = 5;    // if voice is not supported (i.e. mac) then don't show voice page
        }
        for (var i = startingIndex; i < endingIndex; i++) {
            if ($scope.isUnderAge && pageNames[i] === 'diagnostics') {
                continue;
            }

            var item = {
                anchorName: "",
                href: "",
                title: "",
                pageName:""
            };
            item.title = $scope.tabNames[i];
            item.anchorName = $scope.tabNames[i];
            item.pageName = pageNames[i];
            $scope.items.push(item);
        }

        function setPage(pageName) {
            $scope.page = pageName;

            if ($scope.voiceSupported) {
                if (pageName === $scope.pages.voice) {
                    Origin.voice.setInVoiceSettings(true);
                } else {
                    Origin.voice.setInVoiceSettings(false);
                }
            }
        }

        this.setPage = setPage;

        this.navClick = function(item) {
            $state.go('app.settings.page', {
                'page': item.pageName
            });
        };

        // Listen for stateChange incase $state.go('app.settings.page'...) is called externally (from a cpp proxy for example)
        // to setPage and activeItemIndex
        var stopStateChangeListen = $rootScope.$on('$stateChangeStart', function (event, toState, toParams) {
            if (toState.name === 'app.settings.page') {
                setPage(toParams.page);
                $scope.activeItemIndex = _.findIndex($scope.items, function(item) { return item.pageName === toParams.page; });
            }
        });

        $scope.$on('$destroy', stopStateChangeListen);

        this.triggerSavedToasty = function() {
            document.querySelector(".origin-toast-dialog").classList.add("origin-toast-dialog-display");
            setTimeout(function() {
                document.querySelector(".origin-toast-dialog").classList.remove("origin-toast-dialog-display");
            }, 1200);
        };

        // If a specific page was passed with the URL, then go to that page (ie "settings?page=voice")
        // Else, we go to the application page
        if (!!$stateParams.page) {
            $scope.page = $stateParams.page;
        } else {
            if (window.oigParams) {
                $scope.page = $scope.pages.oig;
            } else {
                $scope.page = $scope.pages.application;
            }
        }

        // set the activeItemIndex so the tab will highlight properly
        for (i=0; i < endingIndex; i++){
            if (pageNames[i] === $scope.page){
                // subtract the starting point so that the numbers work if we only show some of the tabs [OIG]
                $scope.activeItemIndex = i - startingIndex;
            }
        }

        this.isSet = function(checkPage) {
            return $scope.page === checkPage;
        };

        this.triggerSavedToasty = function() {
            document.querySelector(".origin-toast-dialog").classList.add("origin-toast-dialog-display");
            setTimeout(function() {
                document.querySelector(".origin-toast-dialog").classList.remove("origin-toast-dialog-display");
            }, 1200);
        };

        this.isClientRunning = function() {
            return Origin.client.isEmbeddedBrowser();
        };
    }

    function originSettings(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                footer: '@footer',
                changeSavedStr: '@changeSaved',
                applicationstr: '@tabApplication',
                diagnosticsstr: '@tabDiagnostics',
                installsstr: '@tabInstalls',
                notificationsstr: '@tabNotifications',
                oigstr: '@tabOig',
                voicestr: '@tabVoice'
            },
            controller: 'OriginSettingsCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('settings/views/settings.html'),
            controllerAs:'OriginSettingsCtrl',
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSettings
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} footer Have questions? <a class=\"otka\">Visit our support site</a> for more in-depth help and information.
     * @param {LocalizedString} change-saved Change Saved
     * @param {LocalizedString} tab-application *update in defaults
     * @param {LocalizedString} tab-diagnostics *update in defaults
     * @param {LocalizedString} tab-installs *update in defaults
     * @param {LocalizedString} tab-notifications *update in defaults
     * @param {LocalizedString} tab-oig *update in defaults
     * @param {LocalizedString} tab-voice *update in defaults
     * @description
     *
     * Settings section
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-settings 
     *              footer="Have questions? <a class=\"otka\">Visit our support site</a> for more in-depth help and information."
     *              change-saved="Change Saved"
     *              tab-application="Application"
     *              tab-diagnostics="Diagnostics"
     *              tab-installs="Installs & Saves"
     *              tab-notifications="Notifications"
     *              tab-oig="Origin In-Game"
     *              tab-voice="Voice"
     *          </origin-settings>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginSettingsCtrl', OriginSettingsCtrl)
        .directive('originSettings', originSettings);
}());
