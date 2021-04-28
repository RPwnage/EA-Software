/**
 * @file application.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-settings-application';

    // Helper object for boolean settings. In the code we deal with boolean values
    // but send to SettingsFactory as 1 or 0.
    var booleanSetting = {
        get: function(number) { return Number(number) === 1 ? true : false; },
        set: function(boolean) { return boolean === true ? 1 : 0; }
    };

    // settings save online in the "General" category
    var onlineSettings = [
        { key: "ShowOriginAfterGameplay", handler: booleanSetting }
    ];

    function OriginSettingsApplicationCtrl($scope, $timeout, SettingsFactory, UtilFactory, ComponentsLogFactory, AuthFactory, EventsHelperFactory, LanguageFactory) {

        var handles = [];

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
		
        // TECH DEBT - ORPLAT-8162 Because of a case mismatch involving the ShowOriginAfterGameplay setting we are allowing online settings to be checked case insensitive.
        function returnOwnPropertyCaseInsensitive(obj, property) {
            //If Case Sensitive Exists return before insensitive.
            if(obj.hasOwnProperty(property)) {
                return property;
            }
            //Check for insensitive
            for (var prop in obj) {
                if(_.isString(prop) && _.isString(property)) {
                    if (prop.toLowerCase() === property.toLowerCase()) {
                        return prop;
                    }
                }
            }
            return null;
        }


        $scope.isUnderAge = Origin.user.underAge();
        $scope.title = UtilFactory.getLocalizedStr($scope.titleStr, CONTEXT_NAMESPACE, 'title-str');
        $scope.defaultSectionStr = UtilFactory.getLocalizedStr($scope.defaultSectionStr, CONTEXT_NAMESPACE, 'defaultsectionstr');
        $scope.defaultSectionHintStr = UtilFactory.getLocalizedStr($scope.defaultSectionHintStr, CONTEXT_NAMESPACE, 'defaultsectionhintstr');
        $scope.defaultOriginStr = UtilFactory.getLocalizedStr($scope.defaultOriginStr, CONTEXT_NAMESPACE, 'defaultletorigindecidestr');
        $scope.defaultHomeStr = UtilFactory.getLocalizedStr($scope.defaultHomeStr, CONTEXT_NAMESPACE, 'defaulthomestr');
        $scope.defaultGameLibraryStr = UtilFactory.getLocalizedStr($scope.defaultGameLibraryStr, CONTEXT_NAMESPACE, 'defaultgamelibrarystr');
        $scope.defaultStoreStr = UtilFactory.getLocalizedStr($scope.defaultStoreStr, CONTEXT_NAMESPACE, 'defaultstorestr');
        $scope.languageSectionStr = UtilFactory.getLocalizedStr($scope.languageSectionStr, CONTEXT_NAMESPACE, 'languagesectionstr');
        $scope.languageSectionHintStr = UtilFactory.getLocalizedStr($scope.languageSectionHintStr, CONTEXT_NAMESPACE, 'languagesectionhintstr');
        $scope.showOriginStr = UtilFactory.getLocalizedStr($scope.showOriginStr, CONTEXT_NAMESPACE, 'showoriginstr');
        $scope.showOriginHintStr = UtilFactory.getLocalizedStr($scope.showOriginHintStr, CONTEXT_NAMESPACE, 'showoriginhintstr');

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
        $scope.goOnlineStr = UtilFactory.getLocalizedStr($scope.goOnlineStr, CONTEXT_NAMESPACE, 'go-online-str');
        $scope.languageOfflineWarningStr = UtilFactory.getLocalizedStr(
            $scope.languageOfflineWarningStr,
            CONTEXT_NAMESPACE,
            'language-offline-warning-str',
            {
                '%go-online-link%': '<a href="javascript:void(0)" ng-click="goOnline()" class="otka">{{goOnlineStr}}</a>'
            }
        );

        $scope.restartChangeLanguageTitleLoc = UtilFactory.getLocalizedStr($scope.restartChangeLanguageTitleStr, CONTEXT_NAMESPACE, 'restart-change-language-title-str') || 'Restart to apply changes';
        $scope.restartChangeLanguageTextLoc = UtilFactory.getLocalizedStr($scope.restartChangeLanguageTextStr, CONTEXT_NAMESPACE, 'restart-change-language-text-str') || 'Changes made to your language settings will be applied when you restart Origin.';
        $scope.restartNowLoc = UtilFactory.getLocalizedStr($scope.restartNowStr, CONTEXT_NAMESPACE, 'restart-now-str') || 'Restart Now';
        $scope.restartCancelLoc = UtilFactory.getLocalizedStr($scope.restartCancelStr, CONTEXT_NAMESPACE, 'restart-cancel-str') || 'Cancel';
        $scope.unableToRestartTitleLoc = UtilFactory.getLocalizedStr($scope.unableToRestartTitleStr, CONTEXT_NAMESPACE, 'unable-to-restart-title-str') || 'SETTINGS';
        $scope.unableToRestartTextLoc = UtilFactory.getLocalizedStr($scope.unableToRestartTextStr, CONTEXT_NAMESPACE, 'unable-to-restart-text-str') || 'Sorry, you cannot change your language at this time.';
        $scope.unableToRestartOkayLoc = UtilFactory.getLocalizedStr($scope.unableToRestartOkayStr, CONTEXT_NAMESPACE, 'unable-to-restart-okay-str') || 'OK';

        /* jshint camelcase: false */

        // web to client settings name map
        $scope.spa_to_client_map = {
            auto_update: "AutoUpdate",
            auto_patch: "AutoPatch",
            beta_opt_in: "BetaOptIn",
            run_on_sys_start: "RunOnSystemStart",
            origin_helper: "LocalHostEnabled",
            origin_responder: "LocalHostResponderEnabled",
            ShowOriginAfterGameplay: "ShowOriginAfterGameplay"
        };

        // defaults
        $scope.app_default = 'decide'; // where to start menu
        $scope.localesSortedArray = [];

        $scope.auto_update = false;
        $scope.auto_patch = false;
        $scope.beta_opt_in = false;
        $scope.run_on_sys_start = false;
        $scope.origin_helper = true;
        $scope.origin_responder = false;
        $scope.ShowOriginAfterGameplay = true;

        $scope.onUpdateSettings = function() {
            //we wrap it in a set time out here, because this callback gets triggered by event from the C++ client
            //if we do not when the app is out of focus, the promise will not resolve until we click back in focus
            setTimeout(function() {
                Origin.client.settings.readSetting("LocalHostResponderEnabled").then(assignSettingToScope('origin_responder'));
                Origin.client.settings.readSetting("LocalHostEnabled").then(assignSettingToScope('origin_helper'));
            }, 0);
        };

        /**
        * Try and take the client online
        * @method goOnline
        */
        $scope.goOnline = function() {
            Origin.client.onlineStatus.goOnline();
        };

        /**
        * Set if the client is online or offline
        * @method setOnlineState
        */
        function setOnlineState() {
            $scope.isOffline = Origin.client.isEmbeddedBrowser() && !AuthFactory.isClientOnline();
        }

        /**
        * Update the online status when an event occurs
        * @method updateOnlineState
        */
        function updateOnlineState() {
            setOnlineState();
            $scope.$digest();
        }

        /**
        * Store the languages and the current locale
        * @param {object} response language map with the key as the locale and value as the string to display
        */
        function storeLanguages(response) {

            // Convert to an array and sort alphabetically
            $scope.localesSortedArray = [];
            for (var key in response) {
                if (response.hasOwnProperty(key)) {
                    var locale = { value: key, label: response[key] };
                    $scope.localesSortedArray.push(locale);
                }
            }

            // Sort it
            $scope.localesSortedArray = _.sortBy($scope.localesSortedArray, 'label');

            // Set the current and fallback locales
            $scope.currentLocale = LanguageFactory.getDashedLocale();
            $scope.fallbackLocale = $scope.currentLocale;
        }

        /**
        * Get the languages and store them
        * @method getLanguages
        */
        function getLanguages() {
            LanguageFactory.getAllLanguages()
                .then(storeLanguages);
        }

        /* initialize html based on settings */
        function getSettings(settingsData) {
            // Starting Area
            if (settingsData) {
                if (settingsData.app_default) {
                    $scope.app_default = settingsData.app_default;
                }

                _.forEach(onlineSettings, function(setting) {
                    var expectedKey = returnOwnPropertyCaseInsensitive(settingsData,setting.key);
                    if(expectedKey) {
                        $scope[setting.key] = setting.handler.get(settingsData[expectedKey]);
                    }
                });
            }
            if (Origin.client.isEmbeddedBrowser()) {

                Origin.client.settings.readSetting("DefaultTab").then(assignSettingToScope('app_default'));

                for (var p in $scope.spa_to_client_map) {
                    if ($scope.spa_to_client_map.hasOwnProperty(p)) {
                        Origin.client.settings.readSetting($scope.spa_to_client_map[p]).then(assignSettingToScope(p));
                    }
                }

                handles.push(Origin.events.on(Origin.events.CLIENT_SETTINGS_RETURN_FROM_DIALOG, $scope.onUpdateSettings));
            }
        }

        function retrieveSettingError(error) {
            ComponentsLogFactory.error('[origin-settings-application]:retrieveSettings FAILED:', error);
        }

        /**
        * Detach the events on destroy
        * @method onDestroy
        */
        function onDestroy() {
            EventsHelperFactory.detachAll(handles);
        }

        SettingsFactory.getGeneralSettings()
            .then(getSettings)
            .then(getLanguages)
            .catch(retrieveSettingError);

        setOnlineState();

        handles.push(AuthFactory.events.on('myauth:clientonlinestatechanged', updateOnlineState));
        $scope.$on('$destroy', onDestroy);

    }

    function originSettingsApplication(ComponentsConfigFactory, SettingsFactory, $compile, ScopeHelper, DialogFactory, LanguageFactory) {
        function originSettingsApplicationLink(scope, elem, attrs, ctrl) {
            var settingsCtrl = ctrl[0];

            /* jshint camelcase: false */
            scope.changeDefault = function() {
                // write it out
                settingsCtrl.triggerSavedToasty();
                Origin.client.settings.writeSetting("DefaultTab", scope.app_default);
            };

            var setLanguage = function(result) {
                if (result.accepted) {
                    settingsCtrl.triggerSavedToasty();
                    //LanguageFactory triggers the client to update the cookie with the new locale
                    //onCookieSet in ClientViewController is then triggered which writes the client language setting.
                    LanguageFactory.setLocale(scope.currentLocale);
                }
                else
                {
                    scope.currentLocale = scope.fallbackLocale;
                    ScopeHelper.digestIfDigestNotInProgress(scope);
                }
            };

            scope.changelanguage = function () {
                if(scope.currentLocale && scope.currentLocale !== scope.fallbackLocale) {
                    Origin.client.user.canLogout().then(function(result) {
                        if(result) {
                            DialogFactory.openPrompt({
                                id: 'origin-language-change-restart-request-id',
                                name: 'origin-language-change-restart-request',
                                title: scope.restartChangeLanguageTitleLoc,
                                description: scope.restartChangeLanguageTextLoc,
                                acceptText: scope.restartNowLoc,
                                acceptEnabled: true,
                                rejectText: scope.restartCancelLoc,
                                defaultBtn: 'yes',
                                callback: setLanguage
                            });
                        } else {
                            scope.currentLocale = scope.fallbackLocale;
                            DialogFactory.openAlert({
                                id: 'origin-language-change-not-allowed-id',
                                name: 'origin-language-change-not-allowed',
                                title: scope.unableToRestartTitleLoc,
                                description: scope.unableToRestartTextLoc,
                                rejectText: scope.unableToRestartOkayLoc
                            });
                        }
                    }).catch(function() {
                        //Error case for if we are on an old client version with a new SPA. This behavior will mimick how we previously handled changing the language.
                        var result = {accepted: true};
                        setLanguage(result);
                    });
                }
            };

            scope.toggleSetting = function(key) {
                scope[key] = !scope[key];
                if (scope.spa_to_client_map[key]) {
                    settingsCtrl.triggerSavedToasty();
                    Origin.client.settings.writeSetting(scope.spa_to_client_map[key], scope[key]);
                }

                var setting = _.find(onlineSettings, function(setting) { return setting.key === key; });
                if (setting) {
                    SettingsFactory.setGeneralSetting(setting.key, setting.handler.set(scope[key]));
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

            // compile the strings and add them to the view
            scope.$watch('isOffline', function() {
                var offlineWarning = elem[0].querySelector('#origin-settings-offline-warning');
                if (offlineWarning) {
                    angular.element(offlineWarning).append(
                        $compile('<span>' + scope.languageOfflineWarningStr + '</span>')(scope)
                    );
                }
            });

        }
        return {
            restrict: 'E',
            scope: {
                titleStr: '@',
                defaultSectionStr: '@defaultsectionstr',
                defaultSectionHintStr: '@defaultsectionhintstr',
                defaultOriginStr: '@defaultletorigindecidestr',
                defaultHomeStr: '@defaulthomestr',
                defaultGameLibraryStr: '@defaultgamelibrarystr',
                defaultStoreStr: '@defaultstorestr',
                languageSectionStr: '@languagesectionstr',
                languageSectionHintStr: '@languagesectionhintstr',
                showOriginStr: '@showoriginstr',
                showOriginHintStr: '@showoriginhintstr',
                clientupdatestr: '@',
                updategamesstr: '@',
                updategameshintstr: '@',
                updateoriginstr: '@',
                updateoriginhintstr: '@',
                originbetastr: '@',
                originbetahintstr: '@',
                startupstr: '@',
                autostartstr: '@',
                autostarthintstr: '@',
                helperstr: '@',
                helperhintstr: '@',
                lhresponderstr: '@',
                shortcutstr: '@',
                shortcuthintstr: '@',
                shortcutcreatestr: '@',
                menushortcutstr: '@',
                menushortcuthintstr: '@',
                menushortcutcreatestr: '@',
                goOnlineStr: '@',
                languageOfflineWarningStr: '@',
                restartChangeLanguageTitleStr: '@',
                restartChangeLanguageTextStr: '@',
                restartNowStr: '@',
                restartCancelStr: '@',
                unableToRestartTitleStr: '@',
                unableToRestartTextStr: '@',
                unableToRestartOkayStr: '@'
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
     * @param {LocalizedString} title-str the title string
     * @param {LocalizedString} defaultsectionstr default section title
     * @param {LocalizedString} defaultsectionhintstr default section hint
     * @param {LocalizedString} defaultletorigindecidestr default section option, default, origin decide
     * @param {LocalizedString} defaulthomestr default section option home
     * @param {LocalizedString} defaultgamelibrarystr default section option library
     * @param {LocalizedString} defaultstorestr default section store
     * @param {LocalizedString} languagesectionstr language section title
     * @param {LocalizedString} languagesectionhintstr language section hint
     * @param {LocalizedString} showoriginstr show origin section title
     * @param {LocalizedString} showoriginhintstr show origin section hint
     * @param {LocalizedString} clientupdatestr client update area title
     * @param {LocalizedString} updategamesstr update game section title
     * @param {LocalizedString} updategameshintstr update game section hint
     * @param {LocalizedString} updateoriginstr update origin section title
     * @param {LocalizedString} updateoriginhintstr update origin section hint
     * @param {LocalizedString} originbetastr participate in beta section title
     * @param {LocalizedString} originbetahintstr participate in beta section hint
     * @param {LocalizedString} startupstr startup section title
     * @param {LocalizedString} autostartstr automatically start origin section title
     * @param {LocalizedString} autostarthintstr automatically start origin section hint
     * @param {LocalizedString} helperstr help section title
     * @param {LocalizedString} helperhintstr help section hint
     * @param {LocalizedString} lhresponderstr origin help service section title
     * @param {LocalizedString} shortcutstr shortcut section title
     * @param {LocalizedString} shortcuthintstr shortcut section hint
     * @param {LocalizedString} shortcutcreatestr shortcut create
     * @param {LocalizedString} menushortcutstr menu shortcut title
     * @param {LocalizedString} menushortcuthintstr menu shortcut hint
     * @param {LocalizedString} menushortcutcreatestr menu shortcut create
     * @param {LocalizedString} go-online-str Go Online
     * @param {LocalizedString} language-offline-warning-str You're Offline, %go-online-link% to modify this option.
     * @param {LocalizedString} restart-change-language-title-str Restart to apply changes
     * @param {LocalizedString} restart-change-language-text-str Changes made to your language settings will be applied when you restart Origin.
     * @param {LocalizedString} restart-now-str Restart Now
     * @param {LocalizedString} restart-cancel-str Cancel 
     * @param {LocalizedString} unable-to-restart-title-str SETTINGS
     * @param {LocalizedString} unable-to-restart-text-str Sorry, you cannot change your language at this time.
     * @param {LocalizedString} unable-to-restart-okay-str OK
     * @description
     *
     * Settings section
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-settings-application title-str="Application Settings"></origin-home-footer>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginSettingsApplicationCtrl', OriginSettingsApplicationCtrl)
        .directive('originSettingsApplication', originSettingsApplication);
}());
