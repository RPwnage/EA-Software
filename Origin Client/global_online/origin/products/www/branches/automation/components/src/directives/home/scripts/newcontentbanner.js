/**
 * @file home/refresh.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-home-newcontentbanner';

    function OriginHomeNewcontentbannerCtrl($scope, UtilFactory, HomeRefreshFactory) {
        function showNotification() {
            $scope.showNotification = true;
            $scope.$digest();
        }

        function init() {
            var refreshInMSecs = parseInt($scope.refreshIntervalMinutes) * 60 * 1000,
            suggestRefreshEvent = HomeRefreshFactory.events.on('suggestRefresh', showNotification);

            $scope.description = UtilFactory.getLocalizedStr($scope.descriptionStr, CONTEXT_NAMESPACE, 'description');

            //don't show the notification batter at the beginning
            $scope.showNotification = false;

            //function triggered when we click on the refresh msg
            $scope.refresh = HomeRefreshFactory.refresh;
            
            $scope.$on('$destroy', suggestRefreshEvent.detach);

            //if we are stale automatically trigger a refresh
            if (HomeRefreshFactory.refreshSuggested()) {
                $scope.refresh(true);
            } else {
                //other wise lets just reset the timer value with the value passed in from CMS (if there is one)
                HomeRefreshFactory.updateRefreshTimer(refreshInMSecs);
            }
        }

        init();
    }

    function originHomeNewcontentbanner(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                descriptionStr: '@description',
                refreshIntervalMinutes: '@refreshinterval'
            },
            controller: 'originHomeNewcontentbannerCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/views/newcontentbanner.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeNewcontentbanner
     * @restrict E
     * @element ANY
     * @scope
     * @param {Number} refreshinterval the time passed before we notify a user of a refresh in minutes
     * @param {LocalizedText} description the feed out of date messaging description
     * @description
     *
     * home section footer
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-newcontentbanner titlestr="Looking for more?" descriptionstr="The more you play games and the more you add friends, the better your recommendations will become."></origin-home-newcontentbanner>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('originHomeNewcontentbannerCtrl', OriginHomeNewcontentbannerCtrl)
        .directive('originHomeNewcontentbanner', originHomeNewcontentbanner);
}());