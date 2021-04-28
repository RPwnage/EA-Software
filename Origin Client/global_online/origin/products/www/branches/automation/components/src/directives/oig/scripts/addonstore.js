/**
 * @file /oig/scripts/addonstore.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-oig-addonstore';

    function OriginOigAddonstoreCtrl($scope, $sce, $stateParams, UtilFactory, GamesDataFactory, GamesCommerceFactory, AppCommFactory) {

        function applyOdcProfile(profile) {
            // Update scope values unless this directive has been merchandized with values already.
            $scope.titlestr = $scope.titlestr || profile.checkoutMessage.titlestr;
            $scope.stripetitlestr = $scope.stripetitlestr || profile.checkoutMessage.stripetitlestr;
            $scope.stripebodystr = $scope.stripebodystr || profile.checkoutMessage.stripebodystr;

            // titlestr will be localized later, after we've retrieved the base game offer data
            $scope.stripeTitleLoc = $sce.trustAsHtml(UtilFactory.getLocalizedStr($scope.stripetitlestr, CONTEXT_NAMESPACE, 'stripetitlestr'));
            $scope.stripeBodyLoc = $sce.trustAsHtml(UtilFactory.getLocalizedStr($scope.stripebodystr, CONTEXT_NAMESPACE, 'stripebodystr'));

            $scope.blacklist = profile.oigOfferBlacklist;
        }

        function getBaseGameOfferId() {
            var masterTitleId = $stateParams.masterTitleId,
                ent = GamesDataFactory.getBaseGameEntitlementByMasterTitleId(masterTitleId);
            if (ent) {
                return Promise.resolve(ent.offerId);
            } else {
                return GamesDataFactory.getPurchasableOfferIdByMasterTitleId(masterTitleId, true);
            }
        }

        function getBaseGameDisplayName(offerId) {
            $scope.baseGameOfferId = offerId;
            return GamesDataFactory.getCatalogInfo([offerId]);
        }

        function updateTitle(catalog) {
            var catalogInfo = catalog[$scope.baseGameOfferId],
                title = catalogInfo ? catalogInfo.i18n.displayName : 'Unknown Game';
            $scope.titleLoc = $sce.trustAsHtml(UtilFactory.getLocalizedStr($scope.titlestr, CONTEXT_NAMESPACE, 'titlestr', {
                "%game%": title
            }));
            $scope.$digest();
        }

        function onCheckoutReady() {
            $scope.checkoutActive = true;
            $scope.$digest();
        }

        function onCheckoutError() {
            $scope.checkoutActive = false;
            $scope.$digest();
        }

        function onDestroy() {
            AppCommFactory.events.off('origin-iframe:ready', onCheckoutReady);
            AppCommFactory.events.off('origin-iframe:error', onCheckoutError);
            AppCommFactory.events.off('origin-dialog-content-checkoutusingbalance:ready', onCheckoutReady);
            AppCommFactory.events.off('origin-dialog-content-checkoutusingbalance:error', onCheckoutError);
        }

        $scope.isStripeVisible = function() {
            return ($scope.stripetitlestr && $scope.stripebodystr);
        };

        AppCommFactory.events.on('origin-iframe:ready', onCheckoutReady);
        AppCommFactory.events.on('origin-iframe:error', onCheckoutError);
        AppCommFactory.events.on('origin-dialog-content-checkoutusingbalance:ready', onCheckoutReady);
        AppCommFactory.events.on('origin-dialog-content-checkoutusingbalance:error', onCheckoutError);
        $scope.$on('$destroy', onDestroy);

        $scope.whitelist = $stateParams.filterOfferIds ? $stateParams.filterOfferIds.split(',') : [];

        GamesCommerceFactory.getOdcProfileById($stateParams.profile)
            .then(applyOdcProfile)
            .then(getBaseGameOfferId)
            .then(getBaseGameDisplayName)
            .then(updateTitle);
    }

    function originOigAddonstore(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                titlestr: '@',
                stripetitlestr: '@',
                striplebodystr: '@'
            },
            controller: 'OriginOigAddonstoreCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('/oig/views/addonstore.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originOigAddonstore
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedText} titlestr The "%game% Extra Content" text
     * @param {LocalizedText} stripetitlestr The stripe title text (optional)
     * @param {LocalizedText} stripebodystr The stripe body text (optional)
     * @description
     *
     * Addon store for in-game purchases
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-oig-addonstore
     *              titlestr="%game% Extra Content"
     *              stripetitlestr="You currently don't have enough FIFA Points to purchase the pack you've selected."
     *              stripebodystr="Please purchase additional points below to complete your transaction."
     *         ></origin-oig-addonstore>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginOigAddonstoreCtrl', OriginOigAddonstoreCtrl)
        .directive('originOigAddonstore', originOigAddonstore);
}());
