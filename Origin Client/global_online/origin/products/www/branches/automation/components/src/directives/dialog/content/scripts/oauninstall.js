/**
 * @file dialog/content/scripts/oauninstall.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-dialog-content-access-uninstall';

    function OriginDialogContentAccessUninstallCtrl($scope, $sce, $state, AppCommFactory, UrlHelper, UtilFactory, GamesDataFactory, GamesCatalogFactory, DialogFactory, SubscriptionFactory, ComponentsConfigHelper) {

        function trans(key, args, count) {
            return UtilFactory.getLocalizedStr($scope[key], CONTEXT_NAMESPACE, key, args, count);
        }

        var offerId = $scope.offerid;
        var catalogInfo = GamesCatalogFactory.getExistingCatalogInfo(offerId);
        var vaultGame = SubscriptionFactory.getVaultGameFromMasterTitleId(catalogInfo.masterTitleId);
        var vaultOfferId = _.get(vaultGame, 'offerId') || '';
        var vaultCatalogInfo = GamesCatalogFactory.getExistingCatalogInfo(vaultOfferId);
        var vaultBaseGameCount = _.get(vaultGame, ['basegames', 'basegame', 'length']) || 1;
        var username = Origin.user.originId();
        var gamename = _.get(catalogInfo, ['i18n', 'displayName']) || trans('unknown-game');
        var vaultgamename = _.get(vaultCatalogInfo, ['i18n', 'displayName']) || trans('unknown-game');
        var isOigContext = angular.isDefined(ComponentsConfigHelper.getOIGContextConfig());
        var self = this;

        // calculate if we owned a lesser edition to be downgraded
        function isDowngradable(offerId) {
            var downgradable = false;
            var ownedOfferIds = GamesDataFactory.baseEntitlementOfferIdList();

            // remove the offerId to uninstall from the list
            ownedOfferIds = ownedOfferIds.filter(function(id) { return id !== offerId && !GamesDataFactory.isSubscriptionEntitlement(id); });

            // get all the offerids from the game
            var gameOfferIds = GamesCatalogFactory.getOfferIdsFromMasterId(catalogInfo.masterTitleId).map(function(o) { return o.offerId; });

            // itersection of owned && master list
            var downgradeOfferId = ownedOfferIds.filter(function(n) {
                return gameOfferIds.indexOf(n) !== -1;
            });
            downgradable = downgradeOfferId.length > 0;

            return downgradable;
        }

        function updateScope() {
            $scope.title = trans('titlestr');
            $scope.description = trans('descriptionstr', { '%username%': username });
            $scope.cancelStr = trans('cancelstr');
            $scope.commandButtons = [
                {
                    icon: 'otkicon-uninstall-and-remove',
                    title: trans('uninstallandremove-titlestr', { '%gamename%': gamename, "%vaultgamename%": vaultgamename }, vaultBaseGameCount),
                    text: trans('uninstallandremove-textstr', { '%gamename%': gamename, "%vaultgamename%": vaultgamename }, vaultBaseGameCount),
                    option: 'uninstall-and-remove'
                },
                {
                    icon: 'otkicon-uninstall',
                    title: trans('uninstallonly-titlestr', { '%gamename%': gamename, "%vaultgamename%": vaultgamename }, vaultBaseGameCount),
                    text: trans('uninstallonly-textstr', { '%gamename%': gamename, "%vaultgamename%": vaultgamename }, vaultBaseGameCount),
                    option: 'uninstall'
                },
            ];

            if (isDowngradable(offerId)) {
                $scope.commandButtons.push({
                    icon: 'otkicon-downgrade',
                    title: trans('downgrade-titlestr', { '%gamename%': gamename, "%vaultgamename%": vaultgamename }, vaultBaseGameCount),
                    text: trans('downgrade-textstr', { '%gamename%': gamename, "%vaultgamename%": vaultgamename }, vaultBaseGameCount),
                    option: 'downgrade'
                });
            }
        }

        function uninstall(silent) {
            // ORPUB-3434: Only send uninstall request from the main SPA to avoid sending it twice if OIG is active.
            if (!isOigContext) {
                Origin.client.games.uninstall(offerId, silent);
            }
        }

        this.handleUninstallAndRemovePrompt = function (result) {
            AppCommFactory.events.off('dialog:hide', self.handleUninstallAndRemovePrompt);
            if (result.accepted) {
                GamesDataFactory.vaultRemove(offerId);
                uninstall(true);
                $state.go('app.game_gamelibrary');
            }
        };

        function promptForUninstallAndRemove() {
            DialogFactory.openPrompt({
                id: 'oauninstall-prompt-uninstallandremove',
                title: trans('uninstallandremove-confirmation-titlestr', { '%gamename%': gamename }),
                description: trans('uninstallandremove-confirmation-textstr'),
                acceptText:trans('uninstallstr'),
                rejectText: trans('cancelstr'),
                defaultBtn: 'no',
                acceptEnabled: true,
                callback: self.handleUninstallAndRemovePrompt
            });
        }

        function onClose(result) {
            switch (result.content.selectedOption) {
                case 'uninstall-and-remove':
                    promptForUninstallAndRemove();
                    break;
                case 'uninstall':
                    uninstall(false);
                    $state.go('app.game_gamelibrary');
                    break;
                case 'downgrade':
                    GamesDataFactory.vaultRemove(offerId);
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

    function originDialogContentAccessUninstall(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            //transclude: true,
            scope: {
                offerid: '@',
                id: '@'
            },
            require: ['^originDialogContentPrompt', '^originDialogContentCommandbuttons'],
            controller: 'OriginDialogContentAccessUninstallCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/oauninstall.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentAccessUninstall
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} titlestr * from defaults - "Remove game"
     * @param {LocalizedString} descriptionstr * from defaults - "Please select an action below to continue."
     * @param {LocalizedString} cancelstr * from defaults - "Cancel"
     * @param {LocalizedString} uninstallandremove-titlestr * from defaults - "Uninstall and remove from game library"
     * @param {LocalizedString} uninstallandremove-textstr * from defaults - "You can always add %gamename% again through Origin Access."
     * @param {LocalizedString} uninstallonly-titlestr * from defaults - "Uninstall only"
     * @param {LocalizedString} uninstallonly-textstr * from defaults - "%gamename%%gamename% will still appear in your game library, but you'll need to download it again to play."
     * @param {LocalizedString} downgrade-titlestr * from defaults - "Revert game edition"
     * @param {LocalizedString} downgrade-textstr * from defaults - "This game will revert back to the version you purchased."
     * @param {LocalizedString} unknown-game * from defaults - "Unknown Game"
     * @param {LocalizedString} remove-game-titlestr * from defaults - "Remove Only This Game from your Library"
     * @param {LocalizedString} remove-game-textstr * from defaults - "You can always re-add %gamename% again through Origin Access"
     * @param {LocalizedString} remove-all-games-titlestr * from defaults - "Remove All Expired Games from Library"
     * @param {LocalizedString} remove-all-games-textstr * from defaults - "This only applies to games you don't have installed"
     * @param {LocalizedString} uninstallandremove-confirmation-titlestr * from defaults - "ARE YOU SURE YOU WANT TO UNINSTALL %gamename%?"
     * @param {LocalizedString} uninstallandremove-confirmation-textstr * from defaults - "All of this game's data will be removed from your computer if you uninstall it. We suggest backing up your game saves separately."
     * @param {LocalizedString} uninstallstr * from defaults - "Uninstall"

     * @description
     *
     * Origin Access uninstall game
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
        .controller('OriginDialogContentAccessUninstallCtrl', OriginDialogContentAccessUninstallCtrl)
        .directive('originDialogContentAccessUninstall', originDialogContentAccessUninstall);

}());
