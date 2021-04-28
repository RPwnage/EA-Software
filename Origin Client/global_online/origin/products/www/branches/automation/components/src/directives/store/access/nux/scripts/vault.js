
/**
 * @file store/access/nux/scripts/vault.js
 */
(function(){
    'use strict';

    function OriginStoreAccessNuxVaultCtrl($scope, $element, DialogFactory, LocalStorageFactory, CheckoutFactory, ComponentsConfigHelper, storeAccessNux) {
        var count = (_.size($element.parent().children()) - 1);

        $scope.getPillCount = function() {
            return new Array(count);
        };

        $scope.currentIndex = $element.index();
        $scope.offerPaths = _.values(_.omit([$scope.offer1, $scope.offer2, $scope.offer3, $scope.offer4], _.isUndefined));

        $scope.nextStage = function() {
            if($element.index() !== count) {
                $element
                    .removeClass(storeAccessNux.class)
                    .next()
                    .addClass(storeAccessNux.class);
                LocalStorageFactory.set(storeAccessNux.key, $element.index()+1);
            } else {
                DialogFactory.close(storeAccessNux.id);
                LocalStorageFactory.delete(storeAccessNux.key);
                if (ComponentsConfigHelper.isOIGContext()) {
                    CheckoutFactory.close();
                }
            }
        };

        if(LocalStorageFactory.get(storeAccessNux.key) === $element.index()) {
            $element.addClass(storeAccessNux.class);
        }
    }

    function originStoreAccessNuxVault(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                titleStr: '@',
                btn: '@',
                description: '@',
                offer1: '@',
                offer2: '@',
                offer3: '@',
                offer4: '@'
            },
            controller: 'OriginStoreAccessNuxVaultCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/access/nux/views/vault.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAccessNuxVault
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {OcdPath} offer1 the first OCD path for the game tiles in this module
     * @param {OcdPath} offer2 the second OCD path for the game tiles in this module
     * @param {OcdPath} offer3 the third OCD path for the game tiles in this module
     * @param {OcdPath} offer4 the fourth OCD path for the game tiles in this module
     * @param {LocalizedString} title-str The title for this module
     * @param {LocalizedString} btn The button text for this module
     * @param {LocalizedString} description The description for this module
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-access-nux-vault>
     *          title-str="some title"
     *          btn="Next"
     *          description="some description"
     *          offer1="/battlefield/battlefield-4/standard-edition"
     *          offer2="/battlefield/battlefield-4/standard-edition"
     *          offer3="/battlefield/battlefield-4/standard-edition"
     *          offer4="/battlefield/battlefield-4/standard-edition"
     *     </origin-store-access-nux-vault>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreAccessNuxVaultCtrl', OriginStoreAccessNuxVaultCtrl)
        .directive('originStoreAccessNuxVault', originStoreAccessNuxVault);
}());
