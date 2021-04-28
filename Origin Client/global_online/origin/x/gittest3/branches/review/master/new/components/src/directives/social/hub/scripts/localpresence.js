(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginSocialHubLocalpresenceCtrl($scope, $timeout, RosterDataFactory, UserDataFactory, AppCommFactory, UtilFactory) {

        var CONTEXT_NAMESPACE = 'origin-social-hub-localpresence';

        $scope.onlineLoc = UtilFactory.getLocalizedStr($scope.onlineStr, CONTEXT_NAMESPACE, 'onlinestr');
        $scope.awayLoc = UtilFactory.getLocalizedStr($scope.awayStr, CONTEXT_NAMESPACE, 'awaystr');
        $scope.invisibleLoc = UtilFactory.getLocalizedStr($scope.invisibleStr, CONTEXT_NAMESPACE, 'invisiblestr');

        $scope.presence = '';
        $scope.presenceStr = '';

        function setPresence(presence, sendRequest) {
            if (presence !== $scope.presence) {
                $scope.presence = presence;
                switch(presence) {
                    case 'online':
                        $scope.presenceStr = $scope.onlineLoc;
                        break;
                    case 'away':
                        $scope.presenceStr = $scope.awayLoc;
                        break;
                    case 'invisible':
                        $scope.presenceStr = $scope.invisibleLoc;
                        break;
                }
                if (sendRequest) {
                    RosterDataFactory.requestPresence(presence);
                }
                $scope.$digest();
            }
        }

        this.onClickedPresenceMenuItem = function (presence) {
            setPresence(presence, true);
        };

        function handlePresenceChange(newPresence) {
            if (!!newPresence.presenceType && newPresence.presenceType === 'UNAVAILABLE'){
                setPresence('invisible', false);
            }
            else if (newPresence.show !== $scope.presence) {
                switch (newPresence.show) {
                    case 'ONLINE':
                        setPresence('online', false);
                        break;
                    case 'AWAY':
                        setPresence('away', false);
                        break;
                }
            }
        }

        function setupInitialPresence() {
            //fire a time out here because handlePresenceChange has a digest in it
            $timeout(handlePresenceChange, 0 , false, RosterDataFactory.getCurrentUserPresence());
            RosterDataFactory.events.on('socialPresenceChanged:' + Origin.user.userPid(), handlePresenceChange);
            $scope.$on('$destroy', function() {
                RosterDataFactory.events.off('socialPresenceChanged:' + Origin.user.userPid(), handlePresenceChange);
            });
        }

        function init() {
            if (Origin.auth.isLoggedIn()) {
                setupInitialPresence();
            }
            else {
                Origin.events.on(Origin.events.AUTH_SUCCESSRETRY, setupInitialPresence);
            }
        }

        if (UserDataFactory.isAuthReady()) {
            init();
        } else {
            AppCommFactory.events.on('auth:ready', init);
            $scope.$on('$destroy', function() {
                AppCommFactory.events.off('auth:ready', init);
                Origin.events.off(Origin.events.AUTH_SUCCESSRETRY, setupInitialPresence);
            });
        }

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
                        var $menu = $(element).find('#origin-social-hub-area-presence-menu');
                        $menu.toggleClass('otkcontextmenu-isvisible');

                        scope.$emit('originSocialPresenceMenuVisible', {});
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

