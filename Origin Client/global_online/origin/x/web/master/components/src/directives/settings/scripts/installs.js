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
        }, 1000);

        function assignSettingToScope(prop) {
            return function(setting) {
                $scope[prop] = setting;
                throttledDigestFunction();
            };
        }
        $scope.inTheCloudStr = UtilFactory.getLocalizedStr($scope.inTheCloudStr, CONTEXT_NAMESPACE, 'inTheCloudStr');
        $scope.savesStr = UtilFactory.getLocalizedStr($scope.savesStr, CONTEXT_NAMESPACE, 'savesStr');
        $scope.savesHintStr = UtilFactory.getLocalizedStr($scope.savesHintStr, CONTEXT_NAMESPACE, 'savesHintStr');
        $scope.onYourCompStr = UtilFactory.getLocalizedStr($scope.onYourCompStr, CONTEXT_NAMESPACE, 'onYourCompStr');
        $scope.gameLibLocStr = UtilFactory.getLocalizedStr($scope.gameLibLocStr, CONTEXT_NAMESPACE, 'gameLibLocStr');
        $scope.directoryStr = UtilFactory.getLocalizedStr($scope.directoryStr, CONTEXT_NAMESPACE, 'directoryStr');
        $scope.changeFolderStr = UtilFactory.getLocalizedStr($scope.changeFolderStr, CONTEXT_NAMESPACE, 'changeFolderStr');
        $scope.restoreDefaultStr = UtilFactory.getLocalizedStr($scope.restoreDefaultStr, CONTEXT_NAMESPACE, 'restoreDefaultStr');
        $scope.legacyGameInstStr = UtilFactory.getLocalizedStr($scope.legacyGameInstStr, CONTEXT_NAMESPACE, 'legacyGameInstStr');
        $scope.legacyGameInstHintStr = UtilFactory.getLocalizedStr($scope.legacyGameInstHintStr, CONTEXT_NAMESPACE, 'legacyGameInstHintStr');
        $scope.browseInstallersStr = UtilFactory.getLocalizedStr($scope.browseInstallersStr, CONTEXT_NAMESPACE, 'browseInstallersStr');
        $scope.deleteAllInstStr = UtilFactory.getLocalizedStr($scope.deleteAllInstStr, CONTEXT_NAMESPACE, 'deleteAllInstStr');
        $scope.gameInstLocStr = UtilFactory.getLocalizedStr($scope.gameInstLocStr, CONTEXT_NAMESPACE, 'gameInstLocStr');
        $scope.gameInstLocHintStr = UtilFactory.getLocalizedStr($scope.gameInstLocHintStr, CONTEXT_NAMESPACE, 'gameInstLocHintStr');

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
            Origin.events.on(Origin.events.CLIENT_SETTINGS_UPDATESETTINGS, $scope.onUpdateSettings);
        }
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
                descriptionStr: '@descriptionstr',
                titleStr: '@titlestr'
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
        .controller('OriginSettingsInstallsCtrl', OriginSettingsInstallsCtrl)
        .directive('originSettingsInstalls', originSettingsInstalls);
}());