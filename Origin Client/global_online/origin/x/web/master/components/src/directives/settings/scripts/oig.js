/**
 * @file oig.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-settings-oig';

    function OriginSettingsIngameCtrl($scope, $timeout, UtilFactory) {
        var throttledDigestFunction = UtilFactory.throttle(function() {
            $timeout(function() {
                $scope.$digest();
            }, 0, false);
        }, 1000);

        function assignSettingToScope(prop) {
            return function(setting) {
                $scope[prop] = setting;
                console.log("hotkey : " + prop + " " + setting);
                throttledDigestFunction();
            };
        }
        $scope.titleStr = UtilFactory.getLocalizedStr($scope.titleStr, CONTEXT_NAMESPACE, 'titleStr');
        $scope.enableOIGStr = UtilFactory.getLocalizedStr($scope.enableOIGStr, CONTEXT_NAMESPACE, 'enableOIGStr');
        $scope.enableOIGHintStr = UtilFactory.getLocalizedStr($scope.enableOIGHintStr, CONTEXT_NAMESPACE, 'enableOIGHintStr');
        $scope.unverifiedStr = UtilFactory.getLocalizedStr($scope.unverifiedStr, CONTEXT_NAMESPACE, 'unverifiedStr');
        $scope.unverifiedHintStr = UtilFactory.getLocalizedStr($scope.unverifiedHintStr, CONTEXT_NAMESPACE, 'unverifiedHintStr');
        $scope.keyboardShortcutStr = UtilFactory.getLocalizedStr($scope.keyboardShortcutStr, CONTEXT_NAMESPACE, 'keyboardShortcutStr');
        $scope.keyboardHintStr = UtilFactory.getLocalizedStr($scope.keyboardHintStr, CONTEXT_NAMESPACE, 'keyboardHintStr');
        $scope.windowPinShortcutStr = UtilFactory.getLocalizedStr($scope.windowPinShortcutStr, CONTEXT_NAMESPACE, 'windowPinShortcutStr');
        $scope.windowPinHintStr = UtilFactory.getLocalizedStr($scope.windowPinHintStr, CONTEXT_NAMESPACE, 'windowPinHintStr');
        $scope.defaultShortcutStr = UtilFactory.getLocalizedStr($scope.defaultShortcutStr, CONTEXT_NAMESPACE, 'defaultShortcutStr');
        if (Origin.utils.os() === 'MAC'){
            $scope.defaultHotkeyStr = UtilFactory.getLocalizedStr($scope.defaultHotkeyStr, CONTEXT_NAMESPACE, 'defaultMACHotkeyStr');
            $scope.defaultHotkeyPinStr = UtilFactory.getLocalizedStr($scope.defaultHotkeyPinStr, CONTEXT_NAMESPACE, 'defaultMACHotkeyPinStr');
        }
        else{
            $scope.defaultHotkeyStr = UtilFactory.getLocalizedStr($scope.defaultHotkeyStr, CONTEXT_NAMESPACE, 'defaultHotkeyStr');
            $scope.defaultHotkeyPinStr = UtilFactory.getLocalizedStr($scope.defaultHotkeyPinStr, CONTEXT_NAMESPACE, 'defaultHotkeyPinStr');
        }
        $scope.shortcutInUseStrBase = UtilFactory.getLocalizedStr($scope.shortcutInUseStr, CONTEXT_NAMESPACE, 'shortcutInUseStr');
        $scope.shortcutInUseStr = $scope.shortcutInUseStrBase;

        var hotKeyAssignment = "",
            settingToDisplayMap = {};
        
        $scope.isGamePlaying = false;

        settingToDisplayMap.HotKeyString = $scope.keyboardShortcutStr;
        settingToDisplayMap.PinHotKeyString = $scope.windowPinShortcutStr;
        settingToDisplayMap.PushToTalkKeyString = UtilFactory.getLocalizedStr("", 'origin-settings-voice', 'pushToTalkShortcutStr');
        
        // default settings
        $scope.enableOIG = true;
        $scope.inGameAllGames = true;
        $scope.hotkeyString = "";
        $scope.hotkeyPinString = "";

        $scope.onUpdateSettings = function() {
            //we wrap it in a set time out here, because this callback gets triggered by event from the C++ client
            //if we do not when the app is out of focus, the promise will not resolve until we click back in focus
            setTimeout(function() {
                Origin.client.settings.readSetting("HotKeyString").then(assignSettingToScope('hotkeyString'));
                Origin.client.settings.readSetting("PinHotKeyString").then(assignSettingToScope('hotkeyPinString'));
            }, 0);
        };
        
        $scope.updateIsGamePlaying = function() {
            $scope.isGamePlaying = Origin.client.games.isGamePlaying();
        };
        
        $scope.onSettingsError = function() {
            var list = Origin.client.settings.hotkeyConflictSwap(); // grab from client the setting that was swapped along with the hot key string 

            $scope.shortcutInUseStr = $scope.shortcutInUseStrBase.replace("%0", settingToDisplayMap[list.setting]);
            $scope.shortcutInUseStr = $scope.shortcutInUseStr.replace("%1", list.keystr);
            
            if (hotKeyAssignment === "HotKey"){
                document.querySelector(".origin-settings-oig-hkerror1").classList.add("otktooltip-isvisible");
            }else
            {
                if (hotKeyAssignment === "PinHotKey"){
                    document.querySelector(".origin-settings-oig-hkerror2").classList.add("otktooltip-isvisible");
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
            Origin.events.on(Origin.events.CLIENT_SETTINGS_UPDATESETTINGS, $scope.onUpdateSettings);
            Origin.events.on(Origin.events.CLIENT_SETTINGS_ERROR, $scope.onSettingsError);
            Origin.events.on(Origin.events.CLIENT_GAMES_CHANGED, $scope.updateIsGamePlaying);
        }

        $scope.setHotKeyInputFocus = function(hasFocus) {
            if(hasFocus)
            {
                $("#oig-leHotKeys").select();
                Origin.client.settings.pinHotkeyInputHasFocus(false);
                Origin.client.settings.hotkeyInputHasFocus(true);
                hotKeyAssignment = "HotKey";
            }
            else
            {
                if (window.getSelection)
                {
                    window.getSelection().empty();
                }
                Origin.client.settings.hotkeyInputHasFocus(false);
                document.querySelector(".origin-settings-oig-hkerror1").classList.remove("otktooltip-isvisible");
                hotKeyAssignment = "";
            }
        };

        $scope.setPinHotKeyInputFocus = function(hasFocus){
            if(hasFocus){
                $("#oig-lePinKeys").select();
                Origin.client.settings.hotkeyInputHasFocus(false);
                Origin.client.settings.pinHotkeyInputHasFocus(true);
                hotKeyAssignment = "PinHotKey";
            }
            else{
                if (window.getSelection){
                    window.getSelection().empty();
                }
                Origin.client.settings.pinHotkeyInputHasFocus(false);
                document.querySelector(".origin-settings-oig-hkerror2").classList.remove("otktooltip-isvisible");
                hotKeyAssignment = "";
            }
        }; 
    }

    function originSettingsIngame(ComponentsConfigFactory) {
        function originSettingsIngameLink(scope, elem, attrs, ctrl) {
            var settingsCtrl = ctrl[0];
            
            // Filter
            $(elem).on('focusin', '.oig-leHotKeys', function() {
                scope.setHotKeyInputFocus(true);
            });
            $(elem).on('focusin', '.oig-lePinKeys', function() {
                scope.setPinHotKeyInputFocus(true);
            });
            $(elem).on('focusout', '.oig-leHotKeys', function() {
                scope.setHotKeyInputFocus(false);
            });
            $(elem).on('focusout', '.oig-lePinKeys', function() {
                scope.setPinHotKeyInputFocus(false);
            });

            
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
        }

        return {
            restrict: 'E',
            scope: {
                descriptionStr: '@descriptionstr',
                titleStr: '@titlestr'
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
        .controller('OriginSettingsIngameCtrl', OriginSettingsIngameCtrl)
        .directive('originSettingsIngame', originSettingsIngame);
}());