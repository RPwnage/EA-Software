/**
 * @file game/settingsbutton.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-game-settings-button';
    var HIDDEN_MENU_ITEMS = ['addasfavoritecta', 'removeasfavoritecta'];

    function OriginGameSettingsButtonCtrl($scope, UtilFactory, GamesContextFactory) {
        $scope.tooltipLabelStr = UtilFactory.getLocalizedStr($scope.tooltiplabel, CONTEXT_NAMESPACE, 'tooltiplabel');
        $scope.availableActions = false;

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

        GamesContextFactory
            .contextMenu($scope.offerId)
            .then(translateMenuStrings);
    }

    function originGameSettingsButton(ComponentsConfigFactory, $document) {

        function originGameSettingsButtonLink(scope, elem) {
            var body = angular.element($document[0].body);
            function toggleMenu(event) {
                event.stopPropagation();
                event.preventDefault();
                scope.isMenuOpen = !scope.isMenuOpen;
                scope.$digest();
            }

            function hideMenu() {
                scope.isMenuOpen = false;
                scope.$digest();
            }

            // @todo: these should go to CSS whenever the CSS is ready for this feature
            scope.top = "33px";
            scope.left ="33px";

            elem.bind('click', toggleMenu);
            elem.bind('contextmenu', toggleMenu);

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