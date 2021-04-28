/**
 * @file diagnostics.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-settings-diagnostics';
    function OriginSettingsDiagnosticsCtrl($scope, $timeout, UtilFactory) {
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

        function convertToString(val) {
            return val.toString();
        }

        $scope.title = UtilFactory.getLocalizedStr($scope.titlestr, CONTEXT_NAMESPACE, 'titlestr');
        $scope.hardwareinfostr = UtilFactory.getLocalizedStr($scope.hardwareinfostr, CONTEXT_NAMESPACE, 'hardwareinfostr');
        $scope.hardwareinfohintstr = UtilFactory.getLocalizedStr($scope.hardwareinfohintstr, CONTEXT_NAMESPACE, 'hardwareinfohintstr');
        $scope.sysinteractstr = UtilFactory.getLocalizedStr($scope.sysinteractstr, CONTEXT_NAMESPACE, 'sysinteractstr');
        $scope.sysinteracthintstr = UtilFactory.getLocalizedStr($scope.sysinteracthintstr, CONTEXT_NAMESPACE, 'sysinteracthintstr');
        $scope.crashreportstr = UtilFactory.getLocalizedStr($scope.crashreportstr, CONTEXT_NAMESPACE, 'crashreportstr');
        $scope.crashreporthintstr = UtilFactory.getLocalizedStr($scope.crashreporthintstr, CONTEXT_NAMESPACE, 'crashreporthintstr');
        $scope.crashmenuaskstr = UtilFactory.getLocalizedStr($scope.crashmenuaskstr, CONTEXT_NAMESPACE, 'crashmenuaskstr');
        $scope.crashmenuautostr = UtilFactory.getLocalizedStr($scope.crashmenuautostr, CONTEXT_NAMESPACE, 'crashmenuautostr');
        $scope.crashmenuneverstr = UtilFactory.getLocalizedStr($scope.crashmenuneverstr, CONTEXT_NAMESPACE, 'crashmenuneverstr');
        $scope.troubleshootingstr = UtilFactory.getLocalizedStr($scope.troubleshootingstr, CONTEXT_NAMESPACE, 'troubleshootingstr');
        $scope.oigloggingstr = UtilFactory.getLocalizedStr($scope.oigloggingstr, CONTEXT_NAMESPACE, 'oigloggingstr');
        $scope.oiglogginghintstr = UtilFactory.getLocalizedStr($scope.oiglogginghintstr, CONTEXT_NAMESPACE, 'oiglogginghintstr');
        $scope.safemodestr = UtilFactory.getLocalizedStr($scope.safemodestr, CONTEXT_NAMESPACE, 'safemodestr');
        $scope.safemodehintstr = UtilFactory.getLocalizedStr($scope.safemodehintstr, CONTEXT_NAMESPACE, 'safemodehintstr');

		/* jshint camelcase: false */

		$scope.spa_to_client_map = {
			hardwareinfo : "TelemetryHWSurveyOptOut",
			oiglogging   : "EnableInGameLogging",
			safemode     : "EnableDownloaderSafeMode",
			telemetry_enabled : "telemetry_enabled"
		};

		// settings defaults
		$scope.crashmenu_setting = '0';
		$scope.hardwareinfo = true;
		$scope.telemetry_enabled = true;
		$scope.oiglogging = true;
		$scope.safemode = true;

		/* initialize html based on settings */
		if (Origin.client.isEmbeddedBrowser()) {
			Origin.client.settings.readSetting("TelemetryCrashDataOptOut")
                .then(convertToString)
                .then(assignSettingToScope('crashmenu_setting'));

			for (var p in $scope.spa_to_client_map){
				if ($scope.spa_to_client_map.hasOwnProperty(p)){
					Origin.client.settings.readSetting($scope.spa_to_client_map[p]).then(assignSettingToScope(p));
				}
			}
		}
	}

    function originSettingsDiagnostics(ComponentsConfigFactory) {
        function originSettingsDiagnosticsLink(scope, elem, attrs, ctrl) {
            var settingsCtrl = ctrl[0];

            /* jshint camelcase: false */
            scope.toggleSetting = function (key) {
                scope[key] = !scope[key];
                // write it out to the server
                if (scope.spa_to_client_map[key]){
                    settingsCtrl.triggerSavedToasty();
                    Origin.client.settings.writeSetting(scope.spa_to_client_map[key], scope[key]);
                }
            };

            scope.changeCrashMenu = function () {
                // write it out to local settings
                settingsCtrl.triggerSavedToasty();
                Origin.client.settings.writeSetting("TelemetryCrashDataOptOut", scope.crashmenu_setting);
            };
        }
        return {
            restrict: 'E',
            scope: {
                titlestr: '@',
                crashmenuaskstr: '@',
                crashmenuautostr: '@',
                crashmenuneverstr: '@',
                crashreporthintstr: '@',
                crashreportstr: '@',
                hardwareinfohintstr: '@',
                hardwareinfostr: '@',
                oiglogginghintstr: '@',
                oigloggingstr: '@',
                safemodehintstr: '@',
                safemodestr: '@',
                sysinteracthintstr: '@',
                sysinteractstr: '@',
                troubleshootingstr: '@'
            },
            require: ['^originSettings'],
            controller: 'OriginSettingsDiagnosticsCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('settings/views/diagnostics.html'),
            link: originSettingsDiagnosticsLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSettingsDiagnostics
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} titlestr the title string
     * @param {LocalizedString} crashmenuaskstr *Update in defaults
     * @param {LocalizedString} crashmenuautostr *Update in defaults
     * @param {LocalizedString} crashmenuneverstr *Update in defaults
     * @param {LocalizedString} crashreporthintstr *Update in defaults
     * @param {LocalizedString} crashreportstr *Update in defaults
     * @param {LocalizedString} hardwareinfohintstr *Update in defaults
     * @param {LocalizedString} hardwareinfostr *Update in defaults
     * @param {LocalizedString} oiglogginghintstr *Update in defaults
     * @param {LocalizedString} oigloggingstr *Update in defaults
     * @param {LocalizedString} safemodehintstr *Update in defaults
     * @param {LocalizedString} safemodestr *Update in defaults
     * @param {LocalizedString} sysinteracthintstr *Update in defaults
     * @param {LocalizedString} sysinteractstr *Update in defaults
     * @param {LocalizedString} troubleshootingstr *Update in defaults
     *
     *
     * @description
     *
     * Settings section
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-settings-diagnostics titlestr="Looking for more?" descriptionstr="The more you play games and the more you add friends, the better your recommendations will become."></origin-settings-diagnostics>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginSettingsDiagnosticsCtrl', OriginSettingsDiagnosticsCtrl)
        .directive('originSettingsDiagnostics', originSettingsDiagnostics);
}());