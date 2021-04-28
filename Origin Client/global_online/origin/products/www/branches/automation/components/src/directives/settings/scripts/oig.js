/**
 * @file oig.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-settings-ingame';
    var CLS_HKERROR1  = '.origin-settings-ingame-hkerror1';
    var CLS_HKERROR2  = '.origin-settings-ingame-hkerror2';
    var CLS_ERROR_ADJUST = "ui-errormsg-adjust";

    function OriginSettingsIngameCtrl($scope, $timeout, UtilFactory, EventsHelperFactory) {
        var throttledDigestFunction = UtilFactory.throttle(function() {
            $timeout(function() {
                $scope.$digest();
            }, 0, false);
        }, 1000),
        eventHandles = [];

        function assignSettingToScope(prop) {
            return function(setting) {
                $scope[prop] = setting;
                console.log("hotkey : " + prop + " " + setting);
                throttledDigestFunction();
            };
        }

        function localize(defaultStr, key) {
            return UtilFactory.getLocalizedStr(defaultStr, CONTEXT_NAMESPACE, key);
        }

        $scope.titleLoc = localize($scope.titlestr, 'titlestr');
        $scope.enableOIGLoc = localize($scope.enableoigstr, 'enableoigstr');
        $scope.enableOIGHintLoc = localize($scope.enableoighintstr, 'enableoighintstr');
        $scope.unverifiedLoc = localize($scope.unverifiedstr, 'unverifiedstr');
        $scope.unverifiedHintLoc = localize($scope.unverifiedhintstr, 'unverifiedhintstr');
        $scope.keyboardShortcutLoc = localize($scope.keyboardshortcutstr, 'keyboardshortcutstr');
        $scope.keyboardHintLoc = localize($scope.keyboardhintstr, 'keyboardhintstr');
        $scope.windowPinShortcutLoc = localize($scope.windowpinshortcutstr, 'windowpinshortcutstr');
        $scope.windowPinHintLoc = localize($scope.windowpinhintstr, 'windowpinhintstr');
        $scope.defaultShortcutLoc = localize($scope.defaultshortcutstr, 'defaultshortcutstr');
        $scope.shortcutInUseLoc = localize($scope.shortcutinusestr, 'shortcutinusestr');

        if (Origin.utils.os() === 'MAC'){
            $scope.defaultHotkeyLoc = localize($scope.defaultmachotkeystr, 'defaultmachotkeystr');
            $scope.defaultHotkeyPinLoc = localize($scope.defaultmachotkeypinstr, 'defaultmachotkeypinstr');
        } else {
            $scope.defaultHotkeyLoc = localize($scope.defaulthotkeystr, 'defaulthotkeystr');
            $scope.defaultHotkeyPinLoc = localize($scope.defaulthotkeypinstr, 'defaulthotkeypinstr');
        }

        var hotKeyAssignment = "",
            settingToDisplayMap = {};

        $scope.isGamePlaying = false;

        settingToDisplayMap.HotKeyString = $scope.keyboardshortcutstr;
        settingToDisplayMap.PinHotKeyString = $scope.windowPinShortcutStr;
        settingToDisplayMap.PushToTalkKeyString = localize("", 'origin-settings-voice', 'pushtotalkshortcutstr');

        // default settings
        $scope.enableOIG = true;
        $scope.inGameAllGames = true;
        $scope.hotkeyString = "";
        $scope.hotkeyPinString = "";
		$scope.hotKeyNoError = true;
		$scope.pinKeyNoError = true;

        $scope.onUpdateSettings = function() {
            //we wrap it in a set time out here, because this callback gets triggered by event from the C++ client
            //if we do not when the app is out of focus, the promise will not resolve until we click back in focus
            setTimeout(function() {
                Origin.client.settings.readSetting("HotKeyString").then(assignSettingToScope('hotkeyString'));
                Origin.client.settings.readSetting("PinHotKeyString").then(assignSettingToScope('hotkeyPinString'));
            }, 0);
        };

        function handleIsGamePlayingResponse(isGamePlaying) {
            $scope.isGamePlaying = isGamePlaying;
            $scope.$digest();
        }
        $scope.updateIsGamePlaying = function() {
            Origin.client.games.isGamePlaying()
                .then(handleIsGamePlayingResponse);
        };

		function addClass(queryName, className)
		{
			document.querySelector(queryName).classList.add(className);
		}

		function removeClass(queryName, className)
		{
			document.querySelector(queryName).classList.remove(className);
		}

		function resetUIErrors() {
			$scope.hotKeyNoError = true;
			$scope.pinKeyNoError = true;
			/* update UI size */
			removeClass(CLS_HKERROR1, CLS_ERROR_ADJUST);
			removeClass(CLS_HKERROR2, CLS_ERROR_ADJUST);

			$scope.$digest();
		}

        $scope.onSettingsError = function() {
			/* show the error for 5 seconds, because it is quite some text */
			$timeout(resetUIErrors, 5000, false);

            if (hotKeyAssignment === "HotKey") {
				$scope.hotKeyNoError = false;
				/* update UI size */
				addClass(CLS_HKERROR1, CLS_ERROR_ADJUST);
			} else {
					if (hotKeyAssignment === "PinHotKey") {
						$scope.pinKeyNoError = false;
						/* update UI size */
						addClass(CLS_HKERROR2, CLS_ERROR_ADJUST);
                }
            }
            $scope.$digest();
        };

        /* initialize html based on settings */
        if (Origin.client.isEmbeddedBrowser()) {
            Origin.client.settings.readSetting("EnableIgo").then(assignSettingToScope('enableOIG'));
            Origin.client.settings.readSetting("in_game_all_games").then(assignSettingToScope('inGameAllGames'));
            Origin.client.settings.readSetting("HotKeyString").then(assignSettingToScope('hotkeyString'));
            Origin.client.settings.readSetting("PinHotKeyString").then(assignSettingToScope('hotkeyPinString'));
            $scope.updateIsGamePlaying();

            eventHandles = [
                Origin.events.on(Origin.events.CLIENT_SETTINGS_UPDATESETTINGS, $scope.onUpdateSettings),
                Origin.events.on(Origin.events.CLIENT_SETTINGS_ERROR, $scope.onSettingsError),
                Origin.events.on(Origin.events.CLIENT_GAMES_CHANGED, $scope.updateIsGamePlaying)            
            ];
        }

        $scope.setHotKeyInputFocus = function(hasFocus) {
            if(hasFocus) {
                Origin.client.settings.pinHotkeyInputHasFocus(false);
                Origin.client.settings.hotkeyInputHasFocus(true);
                hotKeyAssignment = "HotKey";
				$scope.pinKeyNoError = true;
				$scope.hotKeyNoError = true;
            } else {
                if (window.getSelection) {
                    window.getSelection().empty();
                }
                Origin.client.settings.hotkeyInputHasFocus(false);
				hotKeyAssignment = "";
            }
			$scope.$digest();
        };

        $scope.setPinHotKeyInputFocus = function(hasFocus) {
            if(hasFocus) {
                Origin.client.settings.hotkeyInputHasFocus(false);
                Origin.client.settings.pinHotkeyInputHasFocus(true);
                hotKeyAssignment = "PinHotKey";
				$scope.pinKeyNoError = true;
				$scope.hotKeyNoError = true;
            } else {
                if (window.getSelection) {
                    window.getSelection().empty();
                }
                Origin.client.settings.pinHotkeyInputHasFocus(false);
				hotKeyAssignment = "";
            }
			$scope.$digest();
        };

        function onDestroy() {
            EventsHelperFactory.detachAll(eventHandles);
        }

        $scope.$on('$destroy', onDestroy);
    }

    function originSettingsIngame(ComponentsConfigFactory) {
        function originSettingsIngameLink(scope, elem, attrs, ctrl) {
            var settingsCtrl = ctrl[0];

            function onHotKeyFocusIn() {
                scope.setHotKeyInputFocus(true);
            }

            function onPinHotKeyFocusIn() {
                scope.setPinHotKeyInputFocus(true);
            }

            function onHotKeyFocusOut() {
                scope.setHotKeyInputFocus(false);
            }

            function onPinHotKeyFocusOut() {
                scope.setPinHotKeyInputFocus(false);
            }

            function onDestroy() {
                $(elem).off('focusin', '.oig-leHotKeys', onHotKeyFocusIn);
                $(elem).off('focusin', '.oig-lePinKeys', onPinHotKeyFocusIn);
                $(elem).off('focusout', '.oig-leHotKeys', onHotKeyFocusOut);
                $(elem).off('focusout', '.oig-lePinKeys', onPinHotKeyFocusOut);
            }

            // Filter
            $(elem).on('focusin', '.oig-leHotKeys', onHotKeyFocusIn);
            $(elem).on('focusin', '.oig-lePinKeys', onPinHotKeyFocusIn);
            $(elem).on('focusout', '.oig-leHotKeys', onHotKeyFocusOut);
            $(elem).on('focusout', '.oig-lePinKeys', onPinHotKeyFocusOut);


            scope.toggleEnableOIGSetting = function() {
                scope.enableOIG = !scope.enableOIG;
                settingsCtrl.triggerSavedToasty();
                Origin.client.settings.writeSetting("EnableIgo", scope.enableOIG);
            };
            scope.toggleInGameAllGamesSetting = function() {
                scope.inGameAllGames = !scope.inGameAllGames;
                settingsCtrl.triggerSavedToasty();
                Origin.client.settings.writeSetting("in_game_all_games", scope.inGameAllGames);
            };

            scope.$on('$destroy', onDestroy);
        }

        return {
            restrict: 'E',
            scope: {
                titlestr: '@',
                enableoigstr: '@',
                enableoighintstr: '@',
                unverifiedstr: '@',
                unverifiedhintstr: '@',
                keyboardshortcutstr: '@',
                keyboardhintstr: '@',
                windowpinshortcutstr: '@',
                windowpinhintstr: '@',
                defaultshortcutstr: '@',
                shortcutinusestr: '@',
                defaultmachotkeystr: '@',
                defaultmachotkeypinstr: '@',
                defaulthotkeystr: '@',
                defaulthotkeypinstr: '@'
            },
            require: ['^originSettings'],
            controller: 'OriginSettingsIngameCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('settings/views/oig.html'),
            link: originSettingsIngameLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSettingsIngame
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} titlestr *Update in defaults
     * @param {LocalizedString} enableoigstr *Update in defaults
     * @param {LocalizedString} enableoighintstr *Update in defaults
     * @param {LocalizedString} unverifiedstr *Update in defaults
     * @param {LocalizedString} unverifiedhintstr *Update in defaults
     * @param {LocalizedString} keyboardshortcutstr *Update in defaults
     * @param {LocalizedString} keyboardhintstr *Update in defaults
     * @param {LocalizedString} windowpinshortcutstr *Update in defaults
     * @param {LocalizedString} windowpinhintstr *Update in defaults
     * @param {LocalizedString} defaultshortcutstr *Update in defaults
     * @param {LocalizedString} shortcutinusestr *Update in defaults
     * @param {LocalizedString} defaultmachotkeystr *Update in defaults
     * @param {LocalizedString} defaultmachotkeypinstr *Update in defaults
     * @param {LocalizedString} defaulthotkeystr *Update in defaults
     * @param {LocalizedString} defaulthotkeypinstr *Update in defaults
     * 
     * @description
     *
     * Settings section
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-settings-ingame titlestr="Looking for more?" descriptionstr="The more you play games and the more you add friends, the better your recommendations will become."></origin-settings-ingame>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginSettingsIngameCtrl', OriginSettingsIngameCtrl)
        .directive('originSettingsIngame', originSettingsIngame);
}());