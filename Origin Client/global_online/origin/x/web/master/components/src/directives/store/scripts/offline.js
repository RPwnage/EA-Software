/**
 * @file store/offline.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-offline';

    function OriginStoreOfflineCtrl($scope, UtilFactory) {
        $scope.title = UtilFactory.getLocalizedStr($scope.title, CONTEXT_NAMESPACE, 'title');
        $scope.description = UtilFactory.getLocalizedStr($scope.description, CONTEXT_NAMESPACE, 'description');
    }

    function originStoreOffline(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                title: '@',
                description: '@'
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
     * @param {LocalizedString} title the title text
     * @param {LocalizedText} description the description text
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