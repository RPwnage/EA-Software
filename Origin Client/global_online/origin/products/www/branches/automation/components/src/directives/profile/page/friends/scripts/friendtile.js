(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginProfilePageFriendsFriendtileCtrl($scope, $timeout, NavigationFactory, RosterDataFactory, ObserverFactory) {

		RosterDataFactory.getRosterUser($scope.nucleusId).then(function(user) {
			if (!!user) {
				ObserverFactory.create(user._presence)
					.defaultTo({jid: $scope.nucleusId, presenceType: '', show: 'UNAVAILABLE', gameActivity: {}})
					.attachToScope($scope, 'presence');
			}
		});

        $scope.viewProfile = function () {
            NavigationFactory.goUserProfile($scope.nucleusId);
        };

    }


    function originProfilePageFriendsFriendtile(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            controller: 'OriginProfilePageFriendsFriendtileCtrl',
            scope: {
                nucleusId: '@nucleusid',
                username: '@username'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('profile/page/friends/views/friendtile.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originProfilePageFriendsFriendtile
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} nucleusid nucleusId of the user
     * @param {string} username username of the user
     * @description
     *
     * Profile Page - Friends - Friend Tile
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-profile-page-friends-friendtile nucleusid="123456789"
     *             username="{{username}}"
     *         ></origin-profile-page-friends-friendtile>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginProfilePageFriendsFriendtileCtrl', OriginProfilePageFriendsFriendtileCtrl)
        .directive('originProfilePageFriendsFriendtile', originProfilePageFriendsFriendtile);
}());
