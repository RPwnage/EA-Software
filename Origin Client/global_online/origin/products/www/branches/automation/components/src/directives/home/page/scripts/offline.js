/**
 * @file home/page/offline.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-home-page-offline';

    function OriginHomePageOfflineCtrl($scope, UtilFactory) {
        $scope.notAvailableLoc = UtilFactory.getLocalizedStr($scope.notAvailableStr, CONTEXT_NAMESPACE, 'notavailablestr');
    }

    function originHomePageOffline(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                notAvailableStr: '@notavailablestr',
            },
            controller: 'OriginHomePageOfflineCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/page/views/offline.html'),
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomePageOffline
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} notavailablestr "Home is not available in offline mode."
     * @description
     *
     * home offline page
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-page-offline></origin-home-page-offline>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginHomePageOfflineCtrl', OriginHomePageOfflineCtrl)
        .directive('originHomePageOffline', originHomePageOffline);
}());
