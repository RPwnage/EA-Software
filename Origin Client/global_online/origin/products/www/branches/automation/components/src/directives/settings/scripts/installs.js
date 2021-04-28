/**
 * @file installs.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-settings-installs';

    function OriginSettingsInstallsCtrl($scope, $timeout, UtilFactory) {
        var throttledDigestFunction = UtilFactory.throttle(function() {
            $timeout(function() {
                $scope.$digest();
            }, 0, false);
        }, 1000),
        updateSettingsHandle;

        function assignSettingToScope(prop) {
            return function(setting) {
                $scope[prop] = setting;
                throttledDigestFunction();
            };
        }
        $scope.inTheCloudStr = UtilFactory.getLocalizedStr($scope.inthecloudstr, CONTEXT_NAMESPACE, 'inthecloudstr');
        $scope.savesStr = UtilFactory.getLocalizedStr($scope.savesstr, CONTEXT_NAMESPACE, 'savesstr');
        $scope.savesHintStr = UtilFactory.getLocalizedStr($scope.saveshintstr, CONTEXT_NAMESPACE, 'saveshintstr');
        $scope.onYourCompStr = UtilFactory.getLocalizedStr($scope.onyourcompstr, CONTEXT_NAMESPACE, 'onyourcompstr');
        $scope.gameLibLocStr = UtilFactory.getLocalizedStr($scope.gameliblocstr, CONTEXT_NAMESPACE, 'gameliblocstr');
        $scope.directoryStr = UtilFactory.getLocalizedStr($scope.directorystr, CONTEXT_NAMESPACE, 'directorystr');
        $scope.changeFolderStr = UtilFactory.getLocalizedStr($scope.changefolderstr, CONTEXT_NAMESPACE, 'changefolderstr');
        $scope.restoreDefaultStr = UtilFactory.getLocalizedStr($scope.restoredefaultstr, CONTEXT_NAMESPACE, 'restoredefaultstr');
        $scope.legacyGameInstStr = UtilFactory.getLocalizedStr($scope.legacygameinststr, CONTEXT_NAMESPACE, 'legacygameinststr');
        $scope.legacyGameInstHintStr = UtilFactory.getLocalizedStr($scope.legacygameinsthintstr, CONTEXT_NAMESPACE, 'legacygameinsthintstr');
        $scope.browseInstallersStr = UtilFactory.getLocalizedStr($scope.browseinstallersstr, CONTEXT_NAMESPACE, 'browseinstallersstr');
        $scope.deleteAllInstStr = UtilFactory.getLocalizedStr($scope.deleteallinststr, CONTEXT_NAMESPACE, 'deleteallinststr');
        $scope.gameInstLocStr = UtilFactory.getLocalizedStr($scope.gameinstlocstr, CONTEXT_NAMESPACE, 'gameinstlocstr');
        $scope.gameInstLocHintStr = UtilFactory.getLocalizedStr($scope.gameinstlochintstr, CONTEXT_NAMESPACE, 'gameinstlochintstr');
        $scope.notAvailableStr = UtilFactory.getLocalizedStr($scope.notavailablestr, CONTEXT_NAMESPACE, 'notavailablestr');

        // default settings
        $scope.cloudSetting = true;
        $scope.keepInstallersSetting = false;

        $scope.onUpdateSettings = function() {
            //we wrap it in a set time out here, because this callback gets triggered by event from the C++ client
            //if we do not when the app is out of focus, the promise will not resolve until we click back in focus
            setTimeout(function() {
                Origin.client.settings.readSetting("DownloadInPlaceDir").then(assignSettingToScope('gameLibFolderStr'));
                Origin.client.settings.readSetting("CacheDir").then(assignSettingToScope('cacheFolderStr'));
            }, 0);
        };

        $scope.changeFolder = function() {
            Origin.client.installDirectory.chooseDownloadInPlaceLocation();
        };
        $scope.resetDownloadFolder = function() {
            Origin.client.installDirectory.resetDownloadInPlaceLocation();
        };
        $scope.changeCacheFolder = function() {
            Origin.client.installDirectory.chooseInstallerCacheLocation();
        };
        $scope.resetCacheFolder = function() {
            Origin.client.installDirectory.resetInstallerCacheLocation();
        };
        $scope.browseInstallers = function() {
            Origin.client.installDirectory.browseInstallerCache();
        };
        $scope.deleteInstallers = function() {
            Origin.client.installDirectory.deleteInstallers();
        };

        /* initialize html based on settings */
        if (Origin.client.isEmbeddedBrowser()) {
            Origin.client.settings.readSetting("cloud_saves_enabled").then(assignSettingToScope('cloudSetting'));
            Origin.client.settings.readSetting("DownloadInPlaceDir").then(assignSettingToScope('gameLibFolderStr'));
            Origin.client.settings.readSetting("CacheDir").then(assignSettingToScope('cacheFolderStr'));
            Origin.client.settings.readSetting("KeepInstallers").then(assignSettingToScope('keepInstallersSetting'));
            updateSettingsHandle = Origin.events.on(Origin.events.CLIENT_SETTINGS_UPDATESETTINGS, $scope.onUpdateSettings);
        }

        function onDestroy() {
            if(updateSettingsHandle) {
                updateSettingsHandle.detach();                
            }
        }

        $scope.$on('$destroy', onDestroy);
    }

    function originSettingsInstalls(ComponentsConfigFactory) {
        function originSettingsInstallsLink(scope, elem, attrs, ctrl) {
            var settingsCtrl = ctrl[0];

            /* jshint camelcase: false */
            scope.toggleCloudSetting = function() {
                scope.cloudSetting = !scope.cloudSetting;
                settingsCtrl.triggerSavedToasty();
                Origin.client.settings.writeSetting("cloud_saves_enabled", scope.cloudSetting);
            };

            scope.toggleKeepInstallerSetting = function() {
                scope.keepInstallersSetting = !scope.keepInstallersSetting;
                settingsCtrl.triggerSavedToasty();
                Origin.client.settings.writeSetting("KeepInstallers", scope.keepInstallersSetting);
            };


        }
        return {
            restrict: 'E',
            scope: {
                inthecloudstr: '@',
                savesstr: '@',
                saveshintstr: '@',
                onyourcompstr: '@',
                gameliblocstr: '@',
                directorystr: '@',
                changefolderstr: '@',
                restoredefaultstr: '@',
                legacygameinststr: '@',
                legacygameinsthintstr: '@',
                browseinstallersstr: '@',
                deleteallinststr: '@',
                gameinstlocstr: '@',
                gameinstlochintstr: '@',
                notavailablestr: '@'
            },
            require: ['^originSettings'],
            controller: 'OriginSettingsInstallsCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('settings/views/installs.html'),
            link: originSettingsInstallsLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSettingsInstalls
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} browseinstallersstr *Update in defaults
     * @param {LocalizedString} changefolderstr *Update in defaults
     * @param {LocalizedString} deleteallinststr *Update in defaults
     * @param {LocalizedString} directorystr *Update in defaults
     * @param {LocalizedString} gameinstlochintstr *Update in defaults
     * @param {LocalizedString} gameinstlocstr *Update in defaults
     * @param {LocalizedString} gameliblocstr *Update in defaults
     * @param {LocalizedString} inthecloudstr *Update in defaults
     * @param {LocalizedString} legacygameinsthintstr  *Update in defaults
     * @param {LocalizedString} legacygameinststr *Update in defaults
     * @param {LocalizedString} onyourcompstr *Update in defaults
     * @param {LocalizedString} restoredefaultstr *Update in defaults
     * @param {LocalizedString} saveshintstr *Update in defaults
     * @param {LocalizedString} savesstr *Update in defaults
     * @param {LocalizedString} notavailablestr *Update in defaults
     *
     * @description
     * Settings section
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-settings-installs titlestr="Looking for more?" descriptionstr="The more you play games and the more you add friends, the better your recommendations will become."></origin-settings-installs>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginSettingsInstallsCtrl', OriginSettingsInstallsCtrl)
        .directive('originSettingsInstalls', originSettingsInstalls);
}());
