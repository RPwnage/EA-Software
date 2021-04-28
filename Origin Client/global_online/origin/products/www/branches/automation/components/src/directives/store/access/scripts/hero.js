
/**
 * @file store/access/scripts/hero.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-access-hero';

    function OriginStoreAccessHeroCtrl($scope, ComponentsConfigFactory, UtilFactory, PriceInsertionFactory) {
        $scope.leftSlice = ComponentsConfigFactory.getImagePath('store/left-corner.png');
        $scope.rightSlice = ComponentsConfigFactory.getImagePath('store/right-corner.png');

        $scope.strings = {
            subtitle: UtilFactory.getLocalizedStr($scope.subtitle, CONTEXT_NAMESPACE, 'subtitle'),
            buttonText: UtilFactory.getLocalizedStr($scope.buttonText, CONTEXT_NAMESPACE, 'button-text'),
            subscription: UtilFactory.getLocalizedStr($scope.subscription, CONTEXT_NAMESPACE, 'subscription'),
            platform: UtilFactory.getLocalizedStr($scope.platform, CONTEXT_NAMESPACE, 'platform')
        };

        PriceInsertionFactory.insertPriceIntoLocalizedStr($scope, $scope.strings, $scope.titleStr, CONTEXT_NAMESPACE, 'titlestr');
    }

    function originStoreAccessHero(ComponentsConfigFactory) {
        function originStoreAccessHeroLink(scope, element) {
            scope.$on('storeBoxartBackground:loaded', function(){
                angular.element(element).removeClass('origin-store-access-hero-loading');
            });
        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
                image: '@',
                offerId: '@',
                titleStr: '@',
                subtitle: '@',
                buttonText: '@',
                platform: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/access/views/hero.html'),
            controller: OriginStoreAccessHeroCtrl,
            link: originStoreAccessHeroLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAccessHero
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {ImageUrl} image The logo asset above the copy
     * @param {string} offer-id subscription entitlement offerId
     * @param {LocalizedTemplateString} title-str The main title for this hero module
     * @param {LocalizedString} subtitle The subtitle for this hero module
     * @param {LocalizedString} button-text The button text for this hero module
     * @param {LocalizedString} platform The only for windows platform indicator
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-access-hero
     *          image="https://www.someimage.com"
     *          offer-id="Origin.OFR.50.0001171"
     *          title-str="the title"
     *          subtitle="the subtitle"
     *          button-text="some button text"
     *          platform="only for windows">
     *      </origin-store-access-hero>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreAccessHeroCtrl', OriginStoreAccessHeroCtrl)
        .directive('originStoreAccessHero', originStoreAccessHero);
}());