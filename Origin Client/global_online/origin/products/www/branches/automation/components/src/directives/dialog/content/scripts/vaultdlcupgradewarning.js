
/**
 * @file dialog/content/scripts/vaultdlcupgradewarning.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-dialog-vaultdlcupgradewarning';

    function originDialogVaultdlcupgradewarningCtrl($scope, DialogFactory, GamesDataFactory, PurchaseFactory, UtilFactory) {
        $scope.faqHref = UtilFactory.getLocalizedStr($scope.faqHrefOverride, CONTEXT_NAMESPACE, 'faq-href');

        $scope.strings = {
            title: UtilFactory.getLocalizedStr($scope.titleOverride, CONTEXT_NAMESPACE, 'title-override'),
            warning: UtilFactory.getLocalizedStr($scope.warningOverride, CONTEXT_NAMESPACE, 'warning-override'),
            btnOk: UtilFactory.getLocalizedStr($scope.btnOkOverride, CONTEXT_NAMESPACE, 'btn-ok-override'),
            btnCancel: UtilFactory.getLocalizedStr($scope.btnCancel, CONTEXT_NAMESPACE, 'btn-cancel'),
        };

        function setVaultEditionModel(data) {
            var upgradeValue = Origin.utils.getProperty(_.first(_.values(data)), ['platforms', Origin.utils.os(), 'showSubsSaveGameWarning']);
            $scope.showUpgradeWarning = upgradeValue !== null ? upgradeValue : true;
            $scope.$digest();
        }

        function setModel(data) {
            $scope.model = _.first(_.values(data));
            $scope.strings.upgrade = UtilFactory.getLocalizedStr($scope.upgradeOverride, CONTEXT_NAMESPACE, 'upgrade-override', {'%game%': $scope.model.i18n.displayName});
            $scope.$digest();

            GamesDataFactory
                .getCatalogByPath([$scope.model.vaultOcdPath])
                .then(setVaultEditionModel);
        }

        GamesDataFactory
            .getCatalogInfo([$scope.offerId])
            .then(setModel);

        /**
         * Close the dialog
         */
        $scope.clickCancel = function() {
            DialogFactory.close($scope.dialogId);
        };

        /**
         * Close the dialog and initiate vault entitlement with warning acknowledgement
         */
        $scope.clickUpgrade = function() {
            DialogFactory.close($scope.dialogId);
            PurchaseFactory.entitleVaultGame($scope.offerId, true);
        };
    }

    function originDialogVaultdlcupgradewarning(ComponentsConfigFactory) {

        return {
            restrict: 'E',
            scope: {
                dialogId: '@',
                offerId: '@',
                titleOverride: '@',
                upgradeOverride: '@',
                warningOverride: '@',
                btnOkOverride: '@',
                btnCancel: '@'
            },
            controller: originDialogVaultdlcupgradewarningCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/vaultdlcupgradewarning.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogVaultdlcupgradewarning
     * @restrict E
     * @element ANY
     * @scope
     * @description Vault game dlc upgrade/save-game warning modal
     * @param {string} dialog-id Dialog id
     * @param {OfferId} offer-id Offer ID
     * @param {LocalizedString} title-override override string for dialog title
     * @param {LocalizedString} upgrade-override override string for dialog upgrade text
     * @param {LocalizedString} btn-ok-override override string for dialog Ok buton
     * @param {LocalizedString} btn-cancel * default string for dialog Cancel button - merchandized in defaults
     * @param {LocalizedString} warning-override override string for save game warning
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-dialog-vaultdlcupgradewarning dialog-id="vault-dlc-upgrade-warning" offer-id="Origin.OFR.50.0000846" />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('originDialogVaultdlcupgradewarningCtrl', originDialogVaultdlcupgradewarningCtrl)
        .directive('originDialogVaultdlcupgradewarning', originDialogVaultdlcupgradewarning);
}());
