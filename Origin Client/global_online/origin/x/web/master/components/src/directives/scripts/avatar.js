/**
 * @file avatar.js
 */

(function() {
    'use strict';

    function OriginAvatarCtrl($scope, $timeout, UserDataFactory, RosterDataFactory, AuthFactory, ComponentsLogFactory, AppCommFactory) {

        // constants
        var PRES_ONLINE = 'ONLINE',
            PRES_OFFLINE = 'OFFLINE',
            PRES_INGAME = 'INGAME',
            PRES_JOINABLE = 'JOINABLE',
            PRES_BROADCASTING = 'BROADCASTING',
            AVATAR_SIZES = {
                'medium': 66,
                'large': 154,
                'small': 37
            };

        // some defaults
        $scope.userImage = '';
        $scope.originId = '';
        $scope.presence = PRES_OFFLINE;
        $scope.avatarWidthHeight = AVATAR_SIZES[$scope.avatarSize || 'small'];

        $scope.goToProfile = function() {
            AppCommFactory.fire('uiRouter:go', 'app.profile.user', {'id': $scope.nucleusId});
        };

        /**
         * Get the avatar size string
         * @return {string} avatar size string
         * @method getAvatarSize
         */
        function getAvatarSize() {
            if ($scope.avatarSize === 'medium') {
                return 'AVATAR_SZ_MEDIUM';
            } else {
                return 'AVATAR_SZ_SMALL';
            }
        }

        function setPresence(presence) {
            if (!!presence) {
                if (!!presence.presenceType && presence.presenceType === 'UNAVAILABLE') {
                    $scope.presence = PRES_OFFLINE;
                }
                else {
                    $scope.presence = presence.show;
                    if (!!presence.gameActivity && !!presence.gameActivity.productId) {
                        if (presence.gameActivity.joinable) {
                            $scope.presence = PRES_JOINABLE;
                        }
                        else {
                            $scope.presence = PRES_INGAME;
                        }
                    }
                    if (!!presence.gameActivity && !!presence.gameActivity.twitchPresence && presence.gameActivity.twitchPresence.length) {
                        $scope.presence = PRES_BROADCASTING;
                    }
                }
                $timeout(function () { $scope.$digest(); }, 0);
            }
        }

        /**
         * Update the user's presence when it has changed
         * @return {void}
         * @method handlePresenceChange
         */
        function handlePresenceChange(presence) {
            setPresence(presence);
        }

        /**
         * Update the user's online/offline state
         * @return {void}
         * @method onOnlineStateChanged
         */
        function onOnlineStateChanged(onlineObj) {
            if (onlineObj && onlineObj.isOnline) {
                $scope.presence = PRES_ONLINE;
            } else {
                $scope.presence = PRES_OFFLINE;
            }
            $scope.$digest();
        }

        function onVisibilityChange(invisible) {
            if (invisible) {
                $scope.presence = PRES_OFFLINE;
                $scope.$digest();
            } else {
				if ($scope.nucleusId === '' + Origin.user.userPid()) {
					var pres = RosterDataFactory.getCurrentUserPresence();
                    if(!!pres) {
                        setPresence(pres);
                    }
				} else {
					RosterDataFactory.getFriendInfo($scope.nucleusId).then(function (user) {
					    if (!!user && !!user.presence) {
					        setPresence(user.presence);
					    }
					    else {
					        $scope.presence = PRES_OFFLINE;
					        $scope.$digest();
					    }
					});
				}
				$scope.$digest();
            }
        }

        /**
        * clean up after yourself
        * @return {void}
        * @method onDestroy
        */
        function onDestroy() {
            RosterDataFactory.events.off('socialPresenceChanged:' + $scope.nucleusId, handlePresenceChange);
            AuthFactory.events.off('myauth:clientonlinestatechanged', onOnlineStateChanged);
            RosterDataFactory.events.off('visibilityChanged', onVisibilityChange);
        }

        $scope.goToProfile = function() {
            AppCommFactory.events.fire('uiRouter:go', 'app.profile.user', {
                'id': $scope.nucleusId
            });
        };

        /**
         * Get the user information needed for the avatar
         * @return {void}
         * @method getUserInfo
         */
        this.getUserInfo = function() {
            UserDataFactory.getAvatar($scope.nucleusId, getAvatarSize())
                .then(function(response) {
                    $scope.userImage = response;
                    $scope.$digest();
                }).catch(function(error) {
                    ComponentsLogFactory.error('[origin-avatar] UserDataFactory.getAvatar', error.stack);
                });

            // we are looking at the current user
            if ($scope.nucleusId === '' + Origin.user.userPid()) {
                $scope.originId = Origin.user.originId();
				var pres = RosterDataFactory.getCurrentUserPresence();
				setPresence(pres);
            } else {
                RosterDataFactory.getFriendInfo($scope.nucleusId).then(function (user) {
                    if (!!user && !!user.presence) {
                        setPresence(user.presence);
                    }
                    else {
                        $scope.presence = PRES_OFFLINE;
                        $scope.$digest();
                    }
                });
            }
        };

        RosterDataFactory.events.on('socialPresenceChanged:' + $scope.nucleusId, handlePresenceChange);
        AuthFactory.events.on('myauth:clientonlinestatechanged', onOnlineStateChanged);
        RosterDataFactory.events.on('visibilityChanged', onVisibilityChange);
        $scope.$on('$destroy', onDestroy);

    }

    function originAvatar(ComponentsConfigFactory) {

        function originAvatarLink(scope, element, attrs, ctrl) {

            ctrl.getUserInfo();

            if(angular.isDefined(attrs.clickOverride)) {
                scope.onClick = scope.clickOverride;
            } else {
                scope.onClick = scope.goToProfile;
        }
        }

        return {
            restrict: 'E',
            scope: {
                nucleusId: '@nucleusid',
                avatarSize: '@',
                clickOverride: '&'
            },
            controller: 'OriginAvatarCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('views/avatar.html'),
            link: originAvatarLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originAvatar
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} nucleusid the nucleus id of the user whose avatar your want to display
     * @description
     *
     * Shows an avatar for a user
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-avatar nucleusid="12295990004"></origin-avatar>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginAvatarCtrl', OriginAvatarCtrl)
        .directive('originAvatar', originAvatar);
}());
