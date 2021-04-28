
/**
 * @file dialog/content/scripts/vaultupgradewarning.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-dialog-vaultupgradewarning';

    function originDialogVaultupgradewarningCtrl($scope, DialogFactory, GamesDataFactory, NavigationFactory, PurchaseFactory, UtilFactory) {
        $scope.faqHref = UtilFactory.getLocalizedStr($scope.faqHrefOverride, CONTEXT_NAMESPACE, 'faq-href');

        var bodyReplace = {
            '%game-edition%': '<span ng-bind="gameEdition"></span>',
            '%faq-link-start%': '<a class="otka external-link" ng-href="'+$scope.faqHref+'">',
            '%faq-link-end%': '</a>'
        };

        $scope.strings = {
            title: UtilFactory.getLocalizedStr($scope.titleOverride, CONTEXT_NAMESPACE, 'title'),
            body: UtilFactory.getLocalizedStr($scope.bodyOverride, CONTEXT_NAMESPACE, 'body', bodyReplace),
            btnOk: UtilFactory.getLocalizedStr($scope.btnOkOverride, CONTEXT_NAMESPACE, 'btn-ok'),
            btnCancel: UtilFactory.getLocalizedStr($scope.btnCancelOverride, CONTEXT_NAMESPACE, 'btn-cancel'),
        };

        // get gameEdition (will be exposed through compiled body string)
        GamesDataFactory.getCatalogInfo([$scope.offerId])
            .then(function(data) {
                $scope.gameEdition = data[$scope.offerId].i18n.displayName;
            });

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

    function originDialogVaultupgradewarning($compile, ComponentsConfigFactory) {
        function originDialogVaultupgradewarningLink(scope, elem) {
            // modal body string contains html that needs to be compiled and appended to modal
            var modalBodyElement = $compile('<span>'+scope.strings.body+'</span>')(scope);
            elem.find('.vaultupgradewarning-body').append(modalBodyElement);
        }

        return {
            restrict: 'E',
            scope: {
                dialogId: '@',
                offerId: '@',
                titleOverride: '@',
                bodyOverride: '@',
                btnOkOverride: '@',
                btnCancelOverride: '@',
                faqHrefOverride: '@'
            },
            link: originDialogVaultupgradewarningLink,
            controller: originDialogVaultupgradewarningCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/vaultupgradewarning.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogVaultupgradewarning
     * @restrict E
     * @element ANY
     * @scope
     * @description Vault game upgrade save-game warning modal
     * @param {string} dialog-id Dialog id
     * @param {OfferId} offer-id Offer ID
     * @param {LocalizedString} title-override override string for dialog title
     * @param {LocalizedText} body-override override string for dialog body
     * @param {LocalizedString} btn-ok-override override string for dialog Ok buton
     * @param {LocalizedString} btn-cancel-override override string for dialog Cancel button
     * @param {LocalizedString} faq-href-override override string for FAQ href
     *
     *
     * @param {LocalizedString} body *Update in Defaults
     * @param {LocalizedString} btn-cancel *Update in Defaults
     * @param {LocalizedString} btn-ok *Update in Defaults
     * @param {LocalizedString} title *Update in Defaults
     * @param {LocalizedString} faq-href *Update in Defaults
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-dialog-vaultupgradewarning dialog-id="vault-upgrade-savegame-warning" offer-id="Origin.OFR.50.0000846" />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('originDialogVaultupgradewarningCtrl', originDialogVaultupgradewarningCtrl)
        .directive('originDialogVaultupgradewarning', originDialogVaultupgradewarning);
}());
