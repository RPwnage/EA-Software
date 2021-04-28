
/**
 * @file store/access/scripts/comingsoon.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-access-comingsoon';

    function OriginStoreAccessComingsoonCtrl($scope, ComponentsConfigFactory, UtilFactory) {
        $scope.strings = {
            subtitle: UtilFactory.getLocalizedStr($scope.subtitle, CONTEXT_NAMESPACE, 'subtitle'),
            header: UtilFactory.getLocalizedStr($scope.header, CONTEXT_NAMESPACE, 'header'),
            platform: UtilFactory.getLocalizedStr($scope.platform, CONTEXT_NAMESPACE, 'platform')
        };
    }

    function originStoreAccessComingsoon(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                image: '@',
                header: '@',
                subtitle: '@',
                platform: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/access/views/comingsoon.html'),
            controller: OriginStoreAccessComingsoonCtrl
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAccessComingsoon
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {ImageUrl} image The logo asset above the copy
     * @param {LocalizedString} header The main title for this module
     * @param {LocalizedString} subtitle The subtitle for this module
     * @param {LocalizedString} platform The only for windows platform indicator
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-access-comingsoon
     *          image="https://www.someimage.com"
     *          header="the title"
     *          subtitle="the subtitle"
     *          platform="only for windows">
     *      </origin-store-access-comingsoon>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreAccessComingsoonCtrl', OriginStoreAccessComingsoonCtrl)
        .directive('originStoreAccessComingsoon', originStoreAccessComingsoon);
}());