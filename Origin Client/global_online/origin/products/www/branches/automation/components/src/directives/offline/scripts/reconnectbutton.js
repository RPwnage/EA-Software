(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginOfflineReconnectbuttonCtrl($scope, $timeout, UtilFactory, AuthFactory, EventsHelperFactory) {

        var handles = [],
            timoutHandle;

        var CONTEXT_NAMESPACE = 'origin-offline-reconnectbutton';
        $scope.goOnlineLoc = UtilFactory.getLocalizedStr($scope.goOnlineStr, CONTEXT_NAMESPACE, 'goonlinestr');
        $scope.reconnectingLoc = UtilFactory.getLocalizedStr($scope.reconnectingStr, CONTEXT_NAMESPACE, 'reconnectingstr');

        $scope.states = {
            GO_ONLINE: "GO_ONLINE",
            RECONNECTING: "RECONNECTING"
        };

        $scope.state = $scope.states.GO_ONLINE;

        $scope.goOnline = function () {
            $scope.state = $scope.states.RECONNECTING;
            Origin.client.onlineStatus.goOnline();

            timoutHandle = $timeout(function () {
                // Connection timed out, reset state
                $scope.state = $scope.states.GO_ONLINE;
                $scope.$digest();
            }, 2000);

        };

        function destroy() {
            EventsHelperFactory.detachAll(handles);
        }
        $scope.$on('$destroy', destroy);

        function onClientOnlineStateChanged(onlineObj) {
            if (onlineObj && onlineObj.isOnline) {
                // Connected - reset state
                $scope.state = $scope.states.GO_ONLINE;
                $scope.$digest();
                // Cancel timeout
                $timeout.cancel(timoutHandle);
            }
        }
        handles[handles.length] = AuthFactory.events.on('myauth:clientonlinestatechanged', onClientOnlineStateChanged);

    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originOfflineReconnectbutton
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} buttontheme - "light"|"primary"|"transparent"
     * @param {LocalizedString} goonlinestr - "Go Online"
     * @param {LocalizedString} reconnectingstr - "Reconnecting"
     * @description
     *
     * Offline Reconnect Button
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-offline-reconnectbutton
     *            buttontheme="light"
     *            goonlinestr="Go Online"
     *            reconnectingstr="Reconnecting"
     *         ></origin-offline-reconnectbutton>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginOfflineReconnectbuttonCtrl', OriginOfflineReconnectbuttonCtrl)
        .directive('originOfflineReconnectbutton', function (ComponentsConfigFactory) {

            return {
                restrict: 'E',
                controller: 'OriginOfflineReconnectbuttonCtrl',
                scope: {
                    goOnlineStr: '@goonlinestr',
                    reconnectingStr: '@reconnectingstr',
                    buttonTheme: '@buttontheme'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('offline/views/reconnectbutton.html')
            };
        });
}());
