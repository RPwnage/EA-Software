
/**
 * @file globalsitestripe/scripts/truste.js
 */
(function(){
    'use strict';

    var priority = 1000;
    var CONTEXT_NAMESPACE = 'origin-global-sitestripe-truste';

    function OriginGlobalSitestripeTrusteCtrl($scope, $cookies, UtilFactory, GlobalSitestripeFactory) {
        $scope.priority = _.isUndefined($scope.priority) ? priority : Number($scope.priority);
        $scope.message = UtilFactory.getLocalizedStr($scope.message, CONTEXT_NAMESPACE, 'message');
        $scope.reason = UtilFactory.getLocalizedStr($scope.reason, CONTEXT_NAMESPACE, 'reason');
        $scope.cta = UtilFactory.getLocalizedStr($scope.cta, CONTEXT_NAMESPACE, 'cta');

        $scope.hideMessage = function() {
            GlobalSitestripeFactory.hideStripe($scope.$id);
        };

        $scope.btnAction = function() {
            window.truste.eu.clickListener();
        };


        if (!Origin.client.isEmbeddedBrowser() && window.truste && !$cookies.get('notice_preferences')) {
            GlobalSitestripeFactory.showStripe($scope.$id, $scope.priority, $scope.hideMessage, $scope.message, $scope.reason, $scope.cta, $scope.btnAction);
        }
    }

    function originGlobalSitestripeTruste() {
        return {
            restrict: 'E',
            scope: {
                priority: '@',
                message: '@',
                reason: '@',
                cta: '@'
            },
            controller: 'OriginGlobalSitestripeTrusteCtrl'
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originGlobalSitestripeTruste
     * @restrict E
     * @element ANY
     * @scope
     * @description TRUSTe preferences site stripe
     * @param {Number} priority the priority of the site stripe, controls the order which stripes appear
     * @param {LocalizedString} message the bold main title of the message
     * @param {LocalizedString} reason the subtext with details
     * @param {LocalizedString} cta text
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-global-sitestripe-truste />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginGlobalSitestripeTrusteCtrl', OriginGlobalSitestripeTrusteCtrl)
        .directive('originGlobalSitestripeTruste', originGlobalSitestripeTruste);
}());
