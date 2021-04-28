(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginSocialHubLocalpresenceCtrl($scope, $interval, $sce, RosterDataFactory, UserDataFactory, AuthFactory, ComponentsLogFactory, UtilFactory, ObserverFactory) {

        var CONTEXT_NAMESPACE = 'origin-social-hub-localpresence';

        $scope.onlineLoc = UtilFactory.getLocalizedStr($scope.onlineStr, CONTEXT_NAMESPACE, 'onlinestr');
        $scope.awayLoc = UtilFactory.getLocalizedStr($scope.awayStr, CONTEXT_NAMESPACE, 'awaystr');
        $scope.invisibleLoc = UtilFactory.getLocalizedStr($scope.invisibleStr, CONTEXT_NAMESPACE, 'invisiblestr');
        $scope.offlineLoc = UtilFactory.getLocalizedStr($scope.offlineStr, CONTEXT_NAMESPACE, 'offlinestr');
        $scope.inGameLoc = UtilFactory.getLocalizedStr($scope.inGameStr, CONTEXT_NAMESPACE, 'ingamestr');
        $scope.broadcastingLoc = UtilFactory.getLocalizedStr($scope.broadcastingStr, CONTEXT_NAMESPACE, 'broadcastingstr');
        function onXmppConnectionChanged() {
            if (Origin.xmpp.isConnected()) {
                RosterDataFactory.getRosterUser(Origin.user.userPid()).then(function (user) {
                    if (!!user) {
                        ObserverFactory.create(user._presence)
                            .defaultTo({ jid: Origin.user.userPid(), presenceType: '', show: 'UNAVAILABLE', gameActivity: {} })
                            .attachToScope($scope, 'presence');
                    }
                });
            }
        }

        // listen for xmpp connect/disconnect
        ObserverFactory.create(RosterDataFactory.getXmppConnectionWatch())
            .defaultTo({isConnected: Origin.xmpp.isConnected()})
            .attachToScope($scope, 'xmppConnection')
            .attachToScope($scope, onXmppConnectionChanged);

        this.onClickedPresenceMenuItem = function(presence) {
            RosterDataFactory.requestPresence(presence);
        };

        $scope.isPlayingGame = function() {
            return !!$scope.presence && !!$scope.presence.gameActivity && !!$scope.presence.gameActivity.gameTitle && $scope.presence.gameActivity.gameTitle.length;
        };

        $scope.trustAsHtml = function (html) {
            return $sce.trustAsHtml(html);
        };

    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialHubLocalpresence
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} onlinestr "Online"
     * @param {LocalizedString} awaystr "Away"
     * @param {LocalizedString} invisiblestr "Invisible"
     * @param {LocalizedString} offlinestr "Offline"
     * @param {LocalizedString} ingamestr "In Game"
     * @param {LocalizedString} broadcastingstr "Broadcasting"
     * @description
     *
     * origin social hub -> localpresence
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-social-hub-localpresence
     *            onlinestr="Online"
     *            awaystr="Away"
     *            invisiblestr="Invisible"
     *            offlinestr="Offline"
     *            ingamestr="In Game"
     *            broadcastingstr="Broadcasting"
     *         ></origin-social-hub-localpresence>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialHubLocalpresenceCtrl', OriginSocialHubLocalpresenceCtrl)
        .directive('originSocialHubLocalpresence', function(ComponentsConfigFactory) {

            return {
                restrict: 'E',
                controller: 'OriginSocialHubLocalpresenceCtrl',
                scope: {
                    onlineStr: '@onlinestr',
                    awayStr: '@awaystr',
                    invisibleStr: '@invisiblestr',
                    offlineStr: '@offlinestr',
                    inGameStr: '@ingamestr',
                    broadcastingStr: '@broadcastingstr'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/hub/views/localpresence.html'),
                link: function(scope, element, attrs, ctrl) {
                    scope = scope;
                    attrs = attrs;

                    function closeMenu() {
                        var $menu = angular.element(element).find('#origin-social-hub-area-presence-menu');
                        $menu.removeClass('otkcontextmenu-isvisible');
                    }

                    function handleContextmenuClick(event) {
                        ctrl.onClickedPresenceMenuItem(angular.element(event.target).attr('data-value'));
                        closeMenu();
                        return false;
                    }

                    function handleMenuClick(event) {
                        if (scope.xmppConnection.isConnected) {
                            var $menu = angular.element(element).find('#origin-social-hub-area-presence-menu');
                            $menu.toggleClass('otkcontextmenu-isvisible');

                            scope.$emit('originSocialPresenceMenuVisible', {});
                        }
                        event.stopImmediatePropagation();
                        event.preventDefault();
                    }

                    // handle menu item clicks
                    angular.element(element).on('click', '.otkdropdown .otkmenu a', handleContextmenuClick);

                    // show the presence menu
                    angular.element(element).on('click', '.origin-social-hub-area-presence', handleMenuClick);

                    // dismiss the presence menu
                    angular.element(document).on('click', closeMenu);

                    function onDestroy() {
                        angular.element(document).off('click', closeMenu);
                        angular.element(element).off('click', '.origin-social-hub-area-presence', handleMenuClick);
                        angular.element(element).off('click', '.otkdropdown .otkmenu a', handleContextmenuClick);
                    }
                    scope.$on('$destroy', onDestroy);
                    window.addEventListener('unload', onDestroy);


                }
            };

        });
}());
