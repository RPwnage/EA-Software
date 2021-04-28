/**
 * @file game/settingsbutton.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-game-settings-button';
    var HIDDEN_MENU_ITEMS = [
        'showgamedetailscta',
        'viewachievementscta'
    ];

    function OriginGameSettingsButtonCtrl($scope, UtilFactory, GamesContextFactory) {
        $scope.tooltipLabelStr = UtilFactory.getLocalizedStr($scope.tooltiplabel, CONTEXT_NAMESPACE, 'tooltiplabel');
        $scope.availableActions = false;
        var self = this;

        function isMenuItemAvailable(item) {
            return HIDDEN_MENU_ITEMS.indexOf(item.label) < 0;
        }

        function translateMenuItem(item) {
            item.label = UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, item.label);
        }

        /**
         * Translates labels in the given list of menu item objects
         * @return {object}
         */
        function translateMenuStrings(menuItems) {
            $scope.availableActions = menuItems.filter(isMenuItemAvailable);
            angular.forEach($scope.availableActions, translateMenuItem);
        }

        self.reloadContext = function(){
            return GamesContextFactory
                .contextMenu($scope.offerId, CONTEXT_NAMESPACE)
                .then(translateMenuStrings);
        };

        self.reloadContext();
    }

    function originGameSettingsButton(ComponentsConfigFactory, $document) {

        function originGameSettingsButtonLink(scope, element, attrs, ctrl) {
            var body = angular.element($document[0].body);
            function toggleMenu(event) {
                event.stopPropagation();
                event.preventDefault();
                scope.isMenuOpen = !scope.isMenuOpen;
                ctrl.reloadContext().then(function() {
                    scope.$digest();
                });
            }

            function hideMenu() {
                scope.isMenuOpen = false;
                ctrl.reloadContext().then(function() {
                    scope.$digest();
                });
            }

            element.bind('click', toggleMenu);
            element.bind('contextmenu', toggleMenu);

            body.bind('click', hideMenu);
            scope.$on('$destroy', function() {
                body.unbind('click', hideMenu);
            });
        }

        return {
            restrict: 'E',
            scope: {
                offerId: '@offerid',
                tooltiplabel: '@tooltiplabel',
                downloadcta: '@',
                viewgamedetailscta: '@',
                playcta: '@',
                checkforupdatescta: '@',
                pauseupdatecta: '@',
                resumeupdatecta: '@',
                cancelupdatecta: '@',
                checkforrepairgamecta: '@',
                gamerepaircta:'@',
                pauserepaircta: '@',
                resumerepaircta: '@',
                cancelrepaircta: '@',
                installcta: '@',
                uninstallcta: '@',
                updatecta: '@',
                pausegamecta: '@',
                cancelgamecta: '@',
                resumegamecta: '@',
                addtodownloadscta: '@',
                downloadnowcta: '@',
                updatenowcta: '@',
                repairnowcta: '@',
                download: '@',
                preload: '@',
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/views/settingsbutton.html'),
            controller: 'OriginGameSettingsButtonCtrl',
            link: originGameSettingsButtonLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameSettingsButton
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} tooltiplabel the tooltip label for the button
     * @param {LocalizedString} downloadcta download action context menu option
     * @param {LocalizedString} viewgamedetailscta view game details action context menu option
     * @param {LocalizedString} playcta the play action context menu option
     * @param {LocalizedString} checkforupdatescta check for update action context menu option
     * @param {LocalizedString} pauseupdatecta pause game update action context menu option
     * @param {LocalizedString} resumeupdatecta resume game update action context menu option
     * @param {LocalizedString} cancelupdatecta cancel game update action context menu option
     * @param {LocalizedString} checkforrepairgamecta check for repair game action context menu option
     * @param {LocalizedString} gamerepaircta start the repair process action context menu option
     * @param {LocalizedString} pauserepaircta pause the repair process action context menu option
     * @param {LocalizedString} resumerepaircta reume the repair process action context menu option
     * @param {LocalizedString} cancelrepaircta cancel the repair process action context menu option
     * @param {LocalizedString} installcta install game action context menu option
     * @param {LocalizedString} uninstallcta uninstall game action context menu option
     * @param {LocalizedString} updatecta update game action context menu option
     * @param {LocalizedString} pausegamecta pause the specified game action context menu option
     * @param {LocalizedString} cancelgamecta cancel the specified game action context menu option
     * @param {LocalizedString} resumegamecta resume the specified game action context menu option
     * @param {LocalizedString} addtodownloadscta add to downloads action context menu option
     * @param {LocalizedString} downloadnowcta download now action context menu option
     * @param {LocalizedString} updatenowcta update now action context menu option
     * @param {LocalizedString} repairnowcta repair now action context menu option
     * @param {LocalizedString} download the word download as a noun
     * @param {LocalizedString} preload the word preload as a noun
     * @param {LocalizedString} addasfavoritecta *Update in Deafults
     * @param {LocalizedString} gamepropertiescta *Update in Deafults
     * @param {LocalizedString} hidecta *Update in Deafults
     * @param {LocalizedString} removeasfavoritecta *Update in Deafults
     * @param {LocalizedString} showgamedetailscta *Update in Deafults
     * @param {LocalizedString} preloastartdownloadcta *Update in Deafults
     * @param {LocalizedString} starttrialcta *Update in Deafults
     * @param {LocalizedString} viewachievementscta view achievements context menu option
     * @param {LocalizedString} startdownloadcta *Update in defaults
     * @param {LocalizedString} pausedownloadcta *Update in defaults
     * @param {LocalizedString} resumedownloadcta *Update in defaults
     * @param {LocalizedString} canceldownloadcta *Update in defaults
     * @param {LocalizedString} preloadcta Context menu text for starting Pre-load
     * @param {LocalizedString} pausepreloadcta Context menu text for pausing Pre-load
     * @param {LocalizedString} resumepreloadcta Context menu text for resuming Pre-load
     * @param {LocalizedString} cancelpreloadcta Context menu text for cancelling Pre-load
     * @param {LocalizedString} openinodtcta *Update in defaults
     * @param {LocalizedString} cancelcloudsynccta *Update in defaults
     * @param {LocalizedString} repaircta *Update in defaults
     * @param {LocalizedString} restorecta *Update in defaults
     * @param {LocalizedString} pauseinstallcta *Update in defaults
     * @param {LocalizedString} resumeinstallcta *Update in defaults
     * @param {LocalizedString} cancelinstallcta *Update in defaults
     * @param {LocalizedString} updategamecta *Update in defaults
     * @param {LocalizedString} customizeboxartcta *Update in defaults
     * @param {LocalizedString} removefromlibrarycta *Update in defaults
     * @param {LocalizedString} unhidecta *Update in defaults
     * @param {LocalizedString} viewspecialoffercta "View Special Offer"
     * @param {LocalizedString} viewinstorecta "View in Store"
     *
     * @param {string} offerid the offer id to get a context menu for
     * @description
     *
     * A standalone button to left click interact with a game context menu
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-settings-button offerid="OFB-EAST:109549060" tooltiplabel="Favorite"></origin-game-settings-button>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGameSettingsButtonCtrl', OriginGameSettingsButtonCtrl)
        .directive('originGameSettingsButton', originGameSettingsButton);
}());
