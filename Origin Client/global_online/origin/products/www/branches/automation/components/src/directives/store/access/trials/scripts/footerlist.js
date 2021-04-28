
/** 
 * @file store/access/trials/scripts/footerlist.js
 */ 
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-access-trials-footerlist';

    function OriginStoreAccessTrialsFooterlistCtrl($scope, UtilFactory) {
        $scope.strings = {
            header: UtilFactory.getLocalizedStr($scope.header, CONTEXT_NAMESPACE, 'header')
        };
    }
    function originStoreAccessTrialsFooterlist(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            replace: true,
            scope: {
                header: '@'
            },
            controller: OriginStoreAccessTrialsFooterlistCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/access/trials/views/footerlist.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAccessTrialsFooterlist
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} header the header content
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-access-trials-footerlist />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreAccessTrialsFooterlist', originStoreAccessTrialsFooterlist);
}()); 
