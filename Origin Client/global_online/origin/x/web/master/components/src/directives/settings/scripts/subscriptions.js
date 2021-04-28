/**
 * @file subscriptions.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-settings-subscriptions';

    function OriginSettingsSubscriptionsCtrl($scope, UtilFactory) {
        $scope.title = UtilFactory.getLocalizedStr($scope.titleStr, CONTEXT_NAMESPACE, 'titlestr');
        $scope.description = UtilFactory.getLocalizedStr($scope.descriptionStr, CONTEXT_NAMESPACE, 'descriptionstr');
    }

    function originSettingsSubscriptions(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                descriptionStr: '@descriptionstr',
                titleStr: '@titlestr'
            },
            controller: 'OriginSettingsSubscriptionsCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('settings/views/subscriptions.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSettingsSubscriptions
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} titlestr the title string
     * @param {string} descriptionstr the description str
     * @description
     *
     * Settings section 
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-footer titlestr="Looking for more?" descriptionstr="The more you play games and the more you add friends, the better your recommendations will become."></origin-home-footer>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginSettingsSubscriptionsCtrl', OriginSettingsSubscriptionsCtrl)
        .directive('originSettingsSubscriptions', originSettingsSubscriptions);
}());