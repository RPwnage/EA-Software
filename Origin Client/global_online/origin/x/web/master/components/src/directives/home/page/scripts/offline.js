/**
 * @file home/page/offline.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-home-page-offline';

    function OriginHomePageOfflineCtrl($scope, UtilFactory) {
        $scope.title = UtilFactory.getLocalizedStr($scope.title, CONTEXT_NAMESPACE, 'title');
        $scope.description = UtilFactory.getLocalizedStr($scope.description, CONTEXT_NAMESPACE, 'description');
    }

    function originHomePageOffline(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                title: '@',
                description: '@'
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
     * @param {LocalizedString} title the title text
     * @param {LocalizedText} description the description text
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