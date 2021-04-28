(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginSocialHubLocalpresenceCtrl($scope, $timeout, RosterDataFactory, UserDataFactory, AuthFactory, ComponentsLogFactory, UtilFactory) {

        var CONTEXT_NAMESPACE = 'origin-social-hub-localpresence';

        $scope.onlineLoc = UtilFactory.getLocalizedStr($scope.onlineStr, CONTEXT_NAMESPACE, 'onlinestr');
        $scope.awayLoc = UtilFactory.getLocalizedStr($scope.awayStr, CONTEXT_NAMESPACE, 'awaystr');
        $scope.invisibleLoc = UtilFactory.getLocalizedStr($scope.invisibleStr, CONTEXT_NAMESPACE, 'invisiblestr');
        $scope.offlineLoc = UtilFactory.getLocalizedStr($scope.offlineStr, CONTEXT_NAMESPACE, 'offlinestr');

        $scope.presence = '';
        $scope.presenceStr = '';

        function setPresenceString(presence) {
            if (presence !== $scope.presence) {
                $scope.presence = presence;
                switch(presence) {
                    case 'online':
                        $scope.presenceStr = $scope.onlineLoc;
                        break;
                    case 'offline':
                        $scope.presenceStr = $scope.offlineLoc;
                        break;
                    case 'away':
                        $scope.presenceStr = $scope.awayLoc;
                        break;
                    case 'invisible':
                        $scope.presenceStr = $scope.invisibleLoc;
                        break;
                }
                $scope.$digest();
            }
        }

        this.onClickedPresenceMenuItem = function (presence) {
            RosterDataFactory.requestPresence(presence);
            setPresenceString(presence);
        };

        function handlePresenceChange(newPresence) {
            if ( RosterDataFactory.getIsInvisible() || (!!newPresence.presenceType && newPresence.presenceType === 'UNAVAILABLE')) {
                setPresenceString('invisible');
            } else if (newPresence.show !== $scope.presence) {
                switch (newPresence.show) {
                    case 'ONLINE':
                        setPresenceString('online');
                        break;
                    case 'AWAY':
                        setPresenceString('away');
                        break;
                }
            }
        }

        function setupInitialPresence() {
            $scope.clientOnline = true;
            //fire a time out here because handlePresenceChange has a digest in it
            $timeout(handlePresenceChange, 0, false, RosterDataFactory.getCurrentUserPresence());
            RosterDataFactory.events.on('socialPresenceChanged:' + Origin.user.userPid(), handlePresenceChange);
            $scope.$on('$destroy', function() {
                RosterDataFactory.events.off('socialPresenceChanged:' + Origin.user.userPid(), handlePresenceChange);
            });
        }

        function onClientOnlineStateChanged(onlineObj) {
            var online = (onlineObj && onlineObj.isOnline);
            $scope.clientOnline = online;

            if (online) {
                // We don't know the local presence yet.
                // Set it to null while we wait for it to come in.
                $scope.presence = '';
                $scope.presenceStr = '';
                $scope.$digest();
            } else {
                /*
                    The design calls for showing "Offline" when we manually disconnect,
                    and show nothing when we are disconnected from the server for any
                    other reason. Unfortunately the event for going offline arrives
                    right ~after~ the event for getting disconnected from xmpp. In order
                    to differentiate manual offline vs getting disconnected we have to
                    introduce a small delay in handling the disconnect event.
                */
                setTimeout(function () {
                    // setPresence has a digest in it
                    setPresenceString('offline');
                }, 10);
            }
        }

        function onXmppDisconnect() {
            // Disconnected from xmpp
            // Show nothing on the local presence
            $scope.presence = '';
            $scope.presenceStr = '';
            $scope.$digest();
        }

        function onAuthChange(loginObj) {
            if (loginObj && loginObj.isLoggedIn) {
                setupInitialPresence();
            }
        }

        function init(loginObj) {
            if (loginObj && loginObj.isLoggedIn) {
                setupInitialPresence();
            } else {
                AuthFactory.events.on('myauth:change', onAuthChange);
                $scope.$on('$destroy', function() {
                    AuthFactory.events.off('myauth:change', onAuthChange);
                });
            }

            //listen for connection change
            AuthFactory.events.on('myauth:clientonlinestatechangeinitiated', onClientOnlineStateChanged);
            $scope.$on('$destroy', function () {
                AuthFactory.events.off('myauth:clientonlinestatechangeinitiated', onClientOnlineStateChanged);
            });

            // Listen for xmpp disconnect
            RosterDataFactory.events.on('xmppDisconnected', onXmppDisconnect);
            $scope.$on('$destroy', function () {
                RosterDataFactory.events.off('xmppDisconnected', onXmppDisconnect);
            });

        }

        function handleAuthReadyError(error) {
            ComponentsLogFactory.error('ORIGINSOCIALHUBLOCALPRESENCECTRL - authReady error:', error.message);
        }

        AuthFactory.waitForAuthReady()
            .then(init)
            .catch(handleAuthReadyError);
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
                    offlineStr: '@offlinestr'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/hub/views/localpresence.html'),
                link: function (scope, element, attrs, ctrl) {
                    scope = scope;
                    attrs = attrs;

                    // handle menu item clicks
                    $(element).on('click', '.otkdropdown .otkmenu a', function (event) {
                        ctrl.onClickedPresenceMenuItem($(event.target).attr('data-value'));
                    });

                    // show the presence menu
                    $(element).on('click', '.origin-social-hub-area-presence', function (event) {
                        if (scope.clientOnline) {
                            var $menu = $(element).find('#origin-social-hub-area-presence-menu');
                            $menu.toggleClass('otkcontextmenu-isvisible');

                            scope.$emit('originSocialPresenceMenuVisible', {});
                        }
                        event.stopImmediatePropagation();
                        event.preventDefault();
                    });

                    // dismiss the presence menu
                    $(document).on('click', function () {
                        var $menu = $(element).find('#origin-social-hub-area-presence-menu');
                        $menu.removeClass('otkcontextmenu-isvisible');
                    });

                }
            };

        });
}());
