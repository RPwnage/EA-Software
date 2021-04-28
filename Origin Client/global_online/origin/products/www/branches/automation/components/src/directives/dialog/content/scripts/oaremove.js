/**
 * @file dialog/content/scripts/oaremove.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-dialog-content-access-remove';

    function OriginDialogContentAccessRemoveCtrl($scope, $sce, $state, AppCommFactory, UrlHelper, UtilFactory, GamesDataFactory, SubscriptionFactory, GamesCatalogFactory) {

        function trans(key, args, count) {
            return UtilFactory.getLocalizedStr($scope[key], CONTEXT_NAMESPACE, key, args, count);
        }

        // calculate if we owned a lesser edition to be downgraded
        var offerId = $scope.offerid;
        var catalogInfo = GamesCatalogFactory.getExistingCatalogInfo(offerId);
        var vaultGame = SubscriptionFactory.getVaultGameFromMasterTitleId(catalogInfo.masterTitleId);
        var vaultOfferId = _.get(vaultGame, 'offerId') || '';
        var vaultCatalogInfo = GamesCatalogFactory.getExistingCatalogInfo(vaultOfferId);
        var vaultBaseGameCount = _.get(vaultGame, ['basegames', 'basegame', 'length']) || 1;
        var username = Origin.user.originId();
        var gamename = _.get(catalogInfo, ['i18n', 'displayName']) || trans('unknown-game');
        var vaultgamename = _.get(vaultCatalogInfo, ['i18n', 'displayName']) || trans('unknown-game');

        function updateScope() {
            $scope.title = trans('titlestr');
            $scope.description = trans('descriptionstr', { '%username%': username });
            $scope.cancelStr = trans('cancelstr');
            $scope.commandButtons = [
                {
                    icon: 'otkicon-remove',
                    title: trans('remove-game-titlestr', { '%gamename%': gamename, "%vaultgamename%": vaultgamename }, vaultBaseGameCount),
                    text: trans('remove-game-textstr', { '%gamename%': gamename, "%vaultgamename%": vaultgamename }, vaultBaseGameCount),
                    option: 'remove-game'
                },
            ];

            // only remove vault games when the subscription is inactive
            if (!SubscriptionFactory.isActive()) {
                $scope.commandButtons.push({
                    icon: 'otkicon-timer',
                    title: trans('remove-all-games-titlestr'),
                    text: trans('remove-all-games-textstr'),
                    option: 'remove-all-games'
                });
            }
        }

        function removeVaultGamesNotInstalled() {
            var ownedOfferIds = GamesDataFactory.baseEntitlementOfferIdList();
            ownedOfferIds.forEach(function(offerId) {
                if (GamesDataFactory.isSubscriptionEntitlement(offerId) && !GamesDataFactory.isInstalled(offerId)) {
                    GamesDataFactory.vaultRemove(offerId);
                }
            });
        }

        function onClose(result) {
            switch (result.content.selectedOption) {
                case 'remove-game':
                    GamesDataFactory.vaultRemove(offerId);
                    $state.go('app.game_gamelibrary');
                    break;
                case 'remove-all-games':
                    removeVaultGamesNotInstalled();
                    $state.go('app.game_gamelibrary');
                    break;
            }
        }


        function onDestroy() {
            AppCommFactory.events.off('dialog:hide', onClose);
        }

        // subscribe to events
        AppCommFactory.events.on('dialog:hide', onClose);
        $scope.$on('$destroy', onDestroy);
        updateScope();
        $scope.onClose = onClose;
    }

    function originDialogContentAccessRemove(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerid: '@',
                id: '@'
            },
            require: ['^originDialogContentPrompt', '^originDialogContentCommandbuttons'],
            controller: 'OriginDialogContentAccessRemoveCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/oauninstall.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentAccessRemove
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * Origin Access uninstall game
     * @param {LocalizedString} descriptionstr *Update in Defaults
     * @param {LocalizedString} downgrade-titlestr *Update in Defaults
     * @param {LocalizedString} cancelstr *Update in Defaults
     * @param {LocalizedString} titlestr *Update in Defaults
     * @param {LocalizedString} uninstallandremove-titlestr *Update in Defaults
     * @param {LocalizedString} uninstallonly-titlestr *Update in Defaults
     * @param {LocalizedString} remove-game-titlestr *Update in Defaults
     * @param {LocalizedString} remove-game-textstr *Update in Defaults
     * @param {LocalizedString} remove-all-games-titlestr *Update in Defaults
     * @param {LocalizedString} remove-all-games-textstr *Update in Defaults
     * @param {LocalizedString} unknown-game *Update in Defaults
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dialog-content-access-uninstall>
     *         </origin-dialog-content-access-uninstall>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginDialogContentAccessRemoveCtrl', OriginDialogContentAccessRemoveCtrl)
        .directive('originDialogContentAccessRemove', originDialogContentAccessRemove);

}());