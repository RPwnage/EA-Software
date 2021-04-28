
/**
 * @file dialog/content/scripts/subsmacwarning.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-dialog-subsmacwarning';

    function originDialogSubsmacwarningCtrl($scope, DialogFactory, PurchaseFactory, UtilFactory, OriginCheckoutTypesConstant) {
        /** @type buyParams */
        var buyParams = {
                checkoutType: OriginCheckoutTypesConstant.SUBS,
                subsConfirmation: true
            };

        $scope.strings = {
            title: UtilFactory.getLocalizedStr($scope.titleOverride, CONTEXT_NAMESPACE, 'title-override'),
            body: UtilFactory.getLocalizedStr($scope.bodyOverride, CONTEXT_NAMESPACE, 'body-override'),
            btnOk: UtilFactory.getLocalizedStr($scope.btnOkOverride, CONTEXT_NAMESPACE, 'btn-ok-override'),
            btnCancel: UtilFactory.getLocalizedStr($scope.btnCancelOverride, CONTEXT_NAMESPACE, 'btn-cancel')
        };

        /**
         * Close the dialog
         */
        $scope.clickCancel = function() {
            DialogFactory.close($scope.dialogId);
        };

        /**
         * Close the dialog and initiate vault entitlement with warning acknowledgement
         */
        $scope.clickSubscribe = function() {
            DialogFactory.close($scope.dialogId);
            PurchaseFactory.buy($scope.offerId, buyParams);
        };
    }

    function originDialogSubsmacwarning(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                dialogId: '@',
                offerId: '@',
                titleOverride: '@',
                bodyOverride: '@',
                btnOkOverride: '@',
                btnCancelOverride: '@'
            },
            controller: originDialogSubsmacwarningCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/subsmacwarning.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogSubsmacwarning
     * @restrict E
     * @element ANY
     * @scope
     * @description Subscription purchase on mac warning modal
     * @param {string} dialog-id Dialog id
     * @param {OfferId} offer-id Offer ID
     * @param {LocalizedString} title-override override string for dialog title
     * @param {LocalizedText} body-override override string for dialog body
     * @param {LocalizedString} btn-ok-override override string for dialog Ok buton
     * @param {LocalizedString} btn-cancel * "Cancel"
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-dialog-subsmacwarning dialog-id="subs-mac-warning-modal" offer-id="Origin.OFR.50.0001171" />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originDialogSubsmacwarning', originDialogSubsmacwarning);
}());
