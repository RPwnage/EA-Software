/**
 * @file offline/scripts/sitestripe.js
 */
(function () {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-offline-sitestripe';

    function OriginOfflineSitestripeCtrl($scope, $sce, AuthFactory, EventsHelperFactory, UtilFactory) {

        var localize = UtilFactory.getLocalizedStrGenerator(CONTEXT_NAMESPACE),
            handles = [];

        $scope.inClient = Origin.client.isEmbeddedBrowser();
        $scope.strings = {
            offlineTitleLoc: localize($scope.offlineTitleStr, 'offlinetitlestr'),
            offlineReasonLoc: $sce.trustAsHtml(localize($scope.offlineReasonStr, 'offlinereasonstr')),
            goOnlineLoc: localize($scope.goOnlineStr, 'goonlinestr')
        };

        /**
        * Try and take the client online
        * @method goOnline
        */
        $scope.goOnline = function() {
            Origin.client.onlineStatus.goOnline();
        };

        /**
        * Called to set the online status of client
        * @method setOfflineStatus
        */
        function setOfflineStatus() {
            $scope.isOffline = !AuthFactory.isClientOnline();
        }

        /**
        * Update online status based on the clientonlinestatechanged event
        * @param {object} onlineObj
        * @method updateOfflineState
        */
        function updateOfflineState(onlineObj) {
            if (onlineObj) {
                setOfflineStatus();
            }
            $scope.$digest();
        }

        // Determine whether the user is online or offline
        setOfflineStatus();

        /**
        * Called when $scope is destroyed, release event handlers
        * @method onDestroy
        */
        function onDestroy() {
            EventsHelperFactory.detachAll(handles);
        }

        handles = [
            AuthFactory.events.on('myauth:clientonlinestatechanged', updateOfflineState)
        ];

        $scope.$on('$destroy', onDestroy);

    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originOfflineSitestripe
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} offlinetitlestr - "You're in Offline Mode"
     * @param {LocalizedString} offlinereasonstr - "Origin has been set to offline mode. Go Online to reconnect Origin."
     * @param {LocalizedString} goonlinestr - "Go Online"
     * @description
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-offline-sitestripe
     *              offlinetitlestr="You're in Offline Mode"
     *              offlinereasonstr="Origin has been set to offline mode. Go Online to reconnect Origin."
     *              goonlinestr="Go Online"
     *         ></origin-offline-sitestripe>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginOfflineSitestripeCtrl', OriginOfflineSitestripeCtrl)
        .directive('originOfflineSitestripe', function (ComponentsConfigFactory) {

            return {
                restrict: 'E',
                controller: 'OriginOfflineSitestripeCtrl',
                scope: {
                    offlineTitleStr: '@offlinetitlestr',
                    offlineReasonStr: '@offlinereasonstr',
                    goOnlineStr: '@goonlinestr'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('offline/views/sitestripe.html')
            };
        });
}());
