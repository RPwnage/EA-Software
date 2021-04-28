/**
 * @file home/refresh.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-home-newcontentbanner';

    function OriginHomeNewcontentbannerCtrl($scope, UtilFactory, FeedFactory, $state) {
        function showNotification() {
            $scope.showNotification = true;
        }

        function onDestroy() {
            FeedFactory.events.off('suggestRefresh', showNotification);
        }

        function init() {
            var refreshInMSecs = parseInt($scope.refreshIntervalMinutes) * 60 * 1000;

            $scope.description = UtilFactory.getLocalizedStr($scope.descriptionStr, CONTEXT_NAMESPACE, 'description');

            //don't show the notification batter at the beginning
            $scope.showNotification = false;

            //function triggered when we click on the refresh msg
            $scope.refresh = function() {
                var params = angular.copy($state.params);

                FeedFactory.updateRefreshTimer(refreshInMSecs);
                FeedFactory.reset();
                //we need to trigger a dummy param change so ui-router will refresh the same state
                //we cannot use $state.reload as that reloads all the states (social etc)
                params.trigger = params.trigger === 'a' ? "" : 'a';
                $state.go($state.current, params, {
                    location: false
                });

            };

            //listen for the feed factory
            FeedFactory.events.on('suggestRefresh', showNotification);
            $scope.$on('$destroy', onDestroy);

            //if we are stale automatically trigger a refresh
            if (FeedFactory.refreshSuggested()) {
                $scope.refresh();
            } else {
                //other wise lets just reset the timer value with the value passed in from CMS (if there is one)
                FeedFactory.updateRefreshTimer(refreshInMSecs);
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
     *         <origin-home-footer titlestr="Looking for more?" descriptionstr="The more you play games and the more you add friends, the better your recommendations will become."></origin-home-footer>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('originHomeNewcontentbannerCtrl', OriginHomeNewcontentbannerCtrl)
        .directive('originHomeNewcontentbanner', originHomeNewcontentbanner);
}());