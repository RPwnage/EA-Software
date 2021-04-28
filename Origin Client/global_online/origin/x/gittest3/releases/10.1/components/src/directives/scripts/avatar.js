/**
 * @file avatar.js
 */

(function() {
    'use strict';

    function OriginAvatarCtrl($scope, UserDataFactory, RosterDataFactory, ComponentsLogFactory) {

        // constants
        var PRES_ONLINE = 'ONLINE',
            PRES_OFFLINE = 'OFFLINE',
            PRES_INGAME = 'INGAME',
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

        /**
         * Update the user's presence when it has changed
         * @return {void}
         * @method handlePresenceChange
         */
        function handlePresenceChange() {
			var pres;
            if ($scope.nucleusId === '' + Origin.user.userPid()) {
				pres = RosterDataFactory.getCurrentUserPresence();
            } else {
                var info = RosterDataFactory.getRosterFriend($scope.nucleusId);
                pres = info.presence;
            }
			$scope.presence = !!pres.gameActivity && !!pres.gameActivity.productId ? PRES_INGAME : pres.show;
            $scope.$digest();
        }



        /**
         * Update the user's online/offline state
         * @return {void}
         * @method onOnlineStateChanged
         */
        function onOnlineStateChanged(online) {
            if (online) {
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
                handlePresenceChange();
            }
        }

        /**
        * clean up after yourself
        * @return {void}
        * @method onDestroy
        */
        function onDestroy() {
            RosterDataFactory.events.off('socialPresenceChanged:' + $scope.nucleusId, handlePresenceChange);
            Origin.events.off(Origin.events.CLIENT_ONLINESTATECHANGED, onOnlineStateChanged);
            RosterDataFactory.events.off('visibilityChanged', onVisibilityChange);
        }

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

			var pres;

            // we are looking at the current user
            if ($scope.nucleusId === '' + Origin.user.userPid()) {
                $scope.originId = Origin.user.originId();
				pres = RosterDataFactory.getCurrentUserPresence();
			} else {
                // why is this not a promise?
                var info = RosterDataFactory.getRosterFriend($scope.nucleusId);
                if(info) {
                    pres = info.presence;
                    $scope.originId = info.originId;
                }
                else {
                    ComponentsLogFactory.error('[origin-avatar] getRosterFriend -- info is NULL');
                }
            }
            $scope.presence = !!pres.gameActivity && !!pres.gameActivity.productId ? PRES_INGAME : pres.show;
        };

        RosterDataFactory.events.on('socialPresenceChanged:' + $scope.nucleusId, handlePresenceChange);
        Origin.events.on(Origin.events.CLIENT_ONLINESTATECHANGED, onOnlineStateChanged);
        RosterDataFactory.events.on('visibilityChanged', onVisibilityChange);
        $scope.$on('$destroy', onDestroy);

    }

    function originAvatar(ComponentsConfigFactory) {

        function originAvatarLink(scope, element, attrs, ctrl) {
            ctrl.getUserInfo();
        }

        return {
            restrict: 'E',
            scope: {
                nucleusId: '@nucleusid',
                avatarSize: '@'
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