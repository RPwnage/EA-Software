
/** 
 * @file store/access/landing/scripts/footer.js
 */ 
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-access-landing-footer';
    
    function OriginStoreAccessLandingFooterCtrl($scope, UtilFactory, PriceInsertionFactory) {
        $scope.strings = {
            header: UtilFactory.getLocalizedStr($scope.header, CONTEXT_NAMESPACE, 'header'),
            buttonText: UtilFactory.getLocalizedStr($scope.buttonText, CONTEXT_NAMESPACE, 'buttonText')
        };

        PriceInsertionFactory
            .insertPriceIntoLocalizedStr($scope, $scope.strings, $scope.legal, CONTEXT_NAMESPACE, 'legal');

        PriceInsertionFactory
            .insertPriceIntoLocalizedStr($scope, $scope.strings, $scope.subtitle, CONTEXT_NAMESPACE, 'subtitle');
    }
    function originStoreAccessLandingFooter(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            scope: {
                logo: '@',
                header: '@',
                offerId: '@',
                buttonText: '@',
                legal: '@',
                subtitle: '@'
            },
            controller: OriginStoreAccessLandingFooterCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/access/landing/views/footer.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAccessLandingFooter
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {ImageUrl} logo the logo above the header
     * @param {LocalizedString} header the main title
     * @param {String} offer-id The offer id for the join now CTA
     * @param {LocalizedString} button-text The join now CTA text
     * @param {LocalizedTemplateString} legal the legal text below the CTA
     * @param {LocalizedTemplateString} subtitle the subtitle text below the CTA above the legal text
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-access-landing-footer />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreAccessLandingFooter', originStoreAccessLandingFooter);
}()); 
