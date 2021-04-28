/**
 * @file dialog/content/scripts/oaexpired.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-dialog-content-access-expired';
    
    function OriginDialogContentAccessExpiredCtrl($scope, $sce, AppCommFactory, UrlHelper, UtilFactory, GamesDataFactory, SubscriptionFactory, GamesCatalogFactory, PurchaseFactory) {
        
        function trans(key, args) {
            return UtilFactory.getLocalizedStr($scope[key], CONTEXT_NAMESPACE, key, args);
        }
        
        var offerId = $scope.offerid;
        var catalogInfo = GamesCatalogFactory.getExistingCatalogInfo(offerId);
        var gamename = catalogInfo.i18n.displayName || trans('unknown-game');
        
        function updateScope() {
            $scope.title = trans('titlestr');
            $scope.description = trans('descriptionstr', { '%gamename%': gamename });
            $scope.cancelStr = trans('cancelstr');
            $scope.commandButtons = [
                {
                    icon: 'otkicon-refresh',
                    title: trans('renewmembership-titlestr'),
                    text: trans('renewmembership-textstr'),
                    option: 'renew-membership'
                },
                {
                    icon: 'otkicon-store',
                    title: trans('purchase-titlestr'),
                    text: trans('purchase-textstr'),
                    option: 'purchase'
                },
            ];
        }
        
        function onClose(result) {
            console.log(result);
            switch (result.content.selectedOption) {
                case 'renew-membership':
                    PurchaseFactory.renewSubscription();
                    break;
                case 'purchase':
                    GamesDataFactory.buyNow(offerId);
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

    function originDialogContentAccessExpired(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerid: '@',
                id: '@'
            },
            require: ['^originDialogContentPrompt', '^originDialogContentCommandbuttons'],            
            controller: 'OriginDialogContentAccessExpiredCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/oauninstall.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentAccessExpired
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} purchase-titlestr * mechandized in defaulst
     * @param {LocalizedString} descriptionstr * mechandized in defaulst
     * @param {LocalizedString} titlestr * mechandized in defaulst
     * @param {LocalizedString} cancelstr * mechandized in defaulst
     * @param {LocalizedString} renewmembership-titlestr * mechandized in defaulst
     * @description
     *
     * Origin Access dialog when  expired when launching game
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dialog-content-access-expired>
     *         </origin-dialog-content-access-expired>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginDialogContentAccessExpiredCtrl', OriginDialogContentAccessExpiredCtrl)
        .directive('originDialogContentAccessExpired', originDialogContentAccessExpired);

}());