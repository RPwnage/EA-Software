
/** 
 * @file store/access/landing/scripts/trialwrapper.js
 */ 
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-access-landing-trialwrapper';

    function OriginStoreAccessLandingTrialwrapperCtrl($scope, UtilFactory) {
        $scope.strings = {
            header: UtilFactory.getLocalizedStr($scope.header, CONTEXT_NAMESPACE, 'header')
        };
    }
    function originStoreAccessLandingTrialwrapper(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            scope: {
                header: '@'
            },
            controller: OriginStoreAccessLandingTrialwrapperCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/access/landing/views/trialwrapper.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAccessLandingTrialwrapper
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} header The main title 
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-access-landing-trialwrapper />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreAccessLandingTrialwrapper', originStoreAccessLandingTrialwrapper);
}()); 
