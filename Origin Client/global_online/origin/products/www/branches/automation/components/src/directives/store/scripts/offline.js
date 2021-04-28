/**
 * @file store/offline.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-offline';

    function OriginStoreOfflineCtrl($scope, UtilFactory) {
        $scope.notAvailableLoc = UtilFactory.getLocalizedStr($scope.notAvailableStr, CONTEXT_NAMESPACE, 'notavailablestr');
    }

    function originStoreOffline(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                notAvailableStr: '@notavailablestr',
            },
            controller: 'OriginStoreOfflineCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/offline.html'),
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreOffline
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} notavailablestr "The Store is not available in offline mode."
     * @description
     *
     * store offline page
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-offline></origin-store-offline>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreOfflineCtrl', OriginStoreOfflineCtrl)
        .directive('originStoreOffline', originStoreOffline);
}());
