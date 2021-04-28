/**
 * @file application.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-settings-application';

    function OriginSettingsApplicationCtrl($scope, $timeout, SettingsFactory, UtilFactory, ComponentsLogFactory) {
        var throttledDigestFunction = UtilFactory.throttle(function() {
            $timeout(function() {
                $scope.$digest();
            }, 0, false);
        }, 1000);

        function assignSettingToScope(prop) {
            return function(setting) {
                $scope[prop] = setting;
                throttledDigestFunction();
            };
        }

        $scope.title = UtilFactory.getLocalizedStr($scope.titleStr, CONTEXT_NAMESPACE, 'titlestr');
        $scope.description = UtilFactory.getLocalizedStr($scope.descriptionStr, CONTEXT_NAMESPACE, 'descriptionstr');
        $scope.defaultSectionStr = UtilFactory.getLocalizedStr($scope.defaultSectionStr, CONTEXT_NAMESPACE, 'defaultsectionstr');
        $scope.defaultSectionHintStr = UtilFactory.getLocalizedStr($scope.defaultSectionHintStr, CONTEXT_NAMESPACE, 'defaultsectionhintstr');
        $scope.defaultOriginStr = UtilFactory.getLocalizedStr($scope.defaultOriginStr, CONTEXT_NAMESPACE, 'defaultletorigindecidestr');
        $scope.defaultHomeStr = UtilFactory.getLocalizedStr($scope.defaultHomeStr, CONTEXT_NAMESPACE, 'defaulthomestr');
        $scope.defaultGameLibraryStr = UtilFactory.getLocalizedStr($scope.defaultGameLibraryStr, CONTEXT_NAMESPACE, 'defaultgamelibrarystr');
        $scope.defaultStoreStr = UtilFactory.getLocalizedStr($scope.defaultStoreStr, CONTEXT_NAMESPACE, 'defaultstorestr');
        $scope.languageSectionStr = UtilFactory.getLocalizedStr($scope.languageSectionStr, CONTEXT_NAMESPACE, 'languagesectionstr');
        $scope.languageSectionHintStr = UtilFactory.getLocalizedStr($scope.languageSectionHintStr, CONTEXT_NAMESPACE, 'languagesectionhintstr');

        $scope.clientupdatestr = UtilFactory.getLocalizedStr($scope.clientupdatestr, CONTEXT_NAMESPACE, 'clientupdatestr');
        $scope.updategamesstr = UtilFactory.getLocalizedStr($scope.updategamesstr, CONTEXT_NAMESPACE, 'updategamesstr');
        $scope.updategameshintstr = UtilFactory.getLocalizedStr($scope.updategameshintstr, CONTEXT_NAMESPACE, 'updategameshintstr');
        $scope.updateoriginstr = UtilFactory.getLocalizedStr($scope.updateoriginstr, CONTEXT_NAMESPACE, 'updateoriginstr');
        $scope.updateoriginhintstr = UtilFactory.getLocalizedStr($scope.updateoriginhintstr, CONTEXT_NAMESPACE, 'updateoriginhintstr');
        $scope.originbetastr = UtilFactory.getLocalizedStr($scope.originbetastr, CONTEXT_NAMESPACE, 'originbetastr');
        $scope.originbetahintstr = UtilFactory.getLocalizedStr($scope.originbetahintstr, CONTEXT_NAMESPACE, 'originbetahintstr');
        $scope.startupstr = UtilFactory.getLocalizedStr($scope.startupstr, CONTEXT_NAMESPACE, 'startupstr');
        $scope.autostartstr = UtilFactory.getLocalizedStr($scope.autostartstr, CONTEXT_NAMESPACE, 'autostartstr');
        $scope.autostarthintstr = UtilFactory.getLocalizedStr($scope.autostarthintstr, CONTEXT_NAMESPACE, 'autostarthintstr');
        $scope.helperstr = UtilFactory.getLocalizedStr($scope.helperstr, CONTEXT_NAMESPACE, 'helperstr');
        $scope.helperhintstr = UtilFactory.getLocalizedStr($scope.helperhintstr, CONTEXT_NAMESPACE, 'helperhintstr');
        $scope.lhresponderstr = UtilFactory.getLocalizedStr($scope.lhresponderstr, CONTEXT_NAMESPACE, 'lhresponderstr');
        $scope.shortcutstr = UtilFactory.getLocalizedStr($scope.shortcutstr, CONTEXT_NAMESPACE, 'shortcutstr');
        $scope.shortcuthintstr = UtilFactory.getLocalizedStr($scope.shortcuthintstr, CONTEXT_NAMESPACE, 'shortcuthintstr');
        $scope.shortcutcreatestr = UtilFactory.getLocalizedStr($scope.shortcutcreatestr, CONTEXT_NAMESPACE, 'shortcutcreatestr');
        $scope.menushortcutstr = UtilFactory.getLocalizedStr($scope.menushortcutstr, CONTEXT_NAMESPACE, 'menushortcutstr');
        $scope.menushortcuthintstr = UtilFactory.getLocalizedStr($scope.menushortcuthintstr, CONTEXT_NAMESPACE, 'menushortcuthintstr');
        $scope.menushortcutcreatestr = UtilFactory.getLocalizedStr($scope.menushortcutcreatestr, CONTEXT_NAMESPACE, 'menushortcutcreatestr');

        /* jshint camelcase: false */

        // web to client settings name map
        $scope.spa_to_client_map = {
            auto_update: "AutoUpdate",
            auto_patch: "AutoPatch",
            beta_opt_in: "BetaOptIn",
            run_on_sys_start: "RunOnSystemStart",
            origin_helper: "LocalHostEnabled",
            origin_responder: "LocalHostResponderEnabled"
        };

        // defaults
        $scope.app_default = 'decide'; // where to start menu
        $scope.locales = [];

        $scope.auto_update = false;
        $scope.auto_patch = false;
        $scope.beta_opt_in = false;
        $scope.run_on_sys_start = false;
        $scope.origin_helper = true;
        $scope.origin_responder = false;

        $scope.onUpdateSettings = function() {
            //we wrap it in a set time out here, because this callback gets triggered by event from the C++ client
            //if we do not when the app is out of focus, the promise will not resolve until we click back in focus
            setTimeout(function() {
                Origin.client.settings.readSetting("LocalHostResponderEnabled").then(assignSettingToScope('origin_responder'));
                Origin.client.settings.readSetting("LocalHostEnabled").then(assignSettingToScope('origin_helper'));
            }, 0);
        };

        /* initialize html based on settings */
        function getSettings(settingsData) {
            // Starting Area
            if (settingsData) {
                if (settingsData.app_default) {
                    $scope.app_default = settingsData.app_default;
                }
            }
            if (Origin.client.isEmbeddedBrowser()) {
                var list = Origin.client.settings.supportedLanguagesData();
                $scope.locales = list.locales;
                $scope.currentLocale = list.locales[list.currentindex];

                Origin.client.settings.readSetting("DefaultTab").then(assignSettingToScope('app_default'));

                for (var p in $scope.spa_to_client_map) {
                    if ($scope.spa_to_client_map.hasOwnProperty(p)) {
                        Origin.client.settings.readSetting($scope.spa_to_client_map[p]).then(assignSettingToScope(p));
                    }
                }

                Origin.events.on(Origin.events.CLIENT_SETTINGS_RETURN_FROM_DIALOG, $scope.onUpdateSettings);
            }
        }

        function retrieveSettingError(error) {
            ComponentsLogFactory.error('[origin-settings-application]:retrieveSettings FAILED:', error.message);
        }

        SettingsFactory.getGeneralSettings()
            .then(getSettings)
            .catch(retrieveSettingError);

    }

    function originSettingsApplication(ComponentsConfigFactory) {
        function originSettingsApplicationLink(scope, elem, attrs, ctrl) {
            var settingsCtrl = ctrl[0];

            /* jshint camelcase: false */
            scope.changeDefault = function() {
                // write it out
                settingsCtrl.triggerSavedToasty();
                Origin.client.settings.writeSetting("DefaultTab", scope.app_default);
            };

            scope.changeLang = function() {
                // write it out to local settings
                settingsCtrl.triggerSavedToasty();
                Origin.client.settings.writeSetting("Locale", scope.currentLocale.id);
            };

            scope.toggleSetting = function(key) {
                scope[key] = !scope[key];
                if (scope.spa_to_client_map[key]) {
                    settingsCtrl.triggerSavedToasty();
                    Origin.client.settings.writeSetting(scope.spa_to_client_map[key], scope[key]);
                }
            };

            // separate localhost service because this will eventually require a lightbox dialog for the user to Allow
            // this is tricky: IF the user had the 'LocalHostEnabled' setting turned off (from 9.x) then we must ask
            // if they want to turn on the localhost service.  if they agree, we turn on both LocalHostEnabled and LocalHostResponderEnabled.
            scope.toggleLocalhostService = function() {

                if (scope.origin_helper === true){   // if LocalHostEnabled is currently on, then check simply toggle the LocalHostResponderEnabled setting
                    scope.toggleSetting('origin_responder');
                    if (scope.origin_responder === true){
                        Origin.client.settings.startLocalHostResponder();
                    }
                    else{
                        Origin.client.settings.stopLocalHostResponder();
                    }
                }
                else{   // if LocalHostEnabled is OFF, that means the 9.x user opted-OUT so we must ask first before changing the setting
                    Origin.client.settings.startLocalHostResponderFromOptOut();

                    scope.origin_responder = true; // turn setting to ON but don't start the service until user allows it (done in client)
                }
            };
        }
        return {
            restrict: 'E',
            scope: {
                descriptionStr: '@descriptionstr',
                titleStr: '@titlestr'
            },
            require: ['^originSettings'],
            controller: 'OriginSettingsApplicationCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('settings/views/application.html'),
            controllerAs: 'appSettings',
            link: originSettingsApplicationLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSettingsApplication
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} titlestr the title string
     * @param {string} descriptionstr the description str
     * @description
     *
     * Settings section
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-footer titlestr="Looking for more?" descriptionstr="The more you play games and the more you add friends, the better your recommendations will become."></origin-home-footer>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginSettingsApplicationCtrl', OriginSettingsApplicationCtrl)
        .directive('originSettingsApplication', originSettingsApplication);
}());