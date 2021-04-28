/**
 * @file avatar.js
 */

(function() {
    'use strict';

    // constants
    var AVATAR_SIZES = {
            'medium': 66,
            'large': 154,
            'small': 37
        },
        AVATAR_UPDATE_THROTTLE_MS = 500;

    function OriginAvatarCtrl($scope, UserDataFactory, RosterDataFactory, AuthFactory, ComponentsLogFactory, NavigationFactory, EventsHelperFactory, ObserverFactory, ComponentsConfigFactory) {
        var handles;

        // some defaults
        $scope.userImage = ComponentsConfigFactory.getImagePath('social//avatar_placeholder.png');
        $scope.originId = '';
        $scope.avatarWidthHeight = AVATAR_SIZES[$scope.avatarSize || 'small'];

		RosterDataFactory.getRosterUser($scope.nucleusId).then(function(user) {
				if (!!user) {
					ObserverFactory.create(user._presence)
						.defaultTo({jid: $scope.nucleusId, presenceType: '', show: 'UNAVAILABLE', gameActivity: {}})
						.attachToScope($scope, 'presence');

					ObserverFactory.create(user.name)
						.defaultTo({firstName: '', lastName: '', originId: ''})
						.getProperty('originId')
						.attachToScope($scope, 'originId');
				}
		});

        /**
         * Get the avatar size string
         * @return {string} avatar size string
         * @method getAvatarSize
         */
        function getAvatarSize() {
            if ($scope.avatarSize === 'medium') {
                return Origin.defines.avatarSizes.MEDIUM;
            } else {
                return Origin.defines.avatarSizes.SMALL;
            }
        }

        /**
         * Update the user's online/offline state
         * @return {void}
         * @method onOnlineStateChanged
         */
        function onOnlineStateChanged() {
			//TR Jan-06-2016 I'm not sure this function is useful for anything
            $scope.$digest();
        }

        /**
        * clean up after yourself
        * @return {void}
        * @method onDestroy
        */
        function onDestroy() {
            EventsHelperFactory.detachAll(handles);
        }

        /**
         * check if a nucleusId is valid. nucleusId contains all numbers.
         * in our special case, it could be "anonymous"
         * @param nucleusId
         * @returns {boolean}
         */
        function isValideNucluesId(nucleusId) {
            return /^\d+$/.test(nucleusId);
        }

        /**
        * Go to the user's profile if it has a valid nucleusId
        * nucleusId contains all the numbers. in friend recommendation, Anonymous returns as nucleusId
        * @method goToProfile
        * @return {void}
        */
        $scope.goToProfile = function () {
            if (isValideNucluesId($scope.nucleusId)) {
                NavigationFactory.goUserProfile($scope.nucleusId);
            }
        };

        /**
         * Update user avatar from UserDataFactory response
         * @param {string} avatarUrl
         */
        function updateAvatarImage(avatarUrl) {
            $scope.userImage = avatarUrl;
            $scope.$digest();
        }

        function handleUpdateAvatarException(error) {
            ComponentsLogFactory.error('[origin-avatar] UserDataFactory.getAvatar', error);
        }

        /**
         * Get the user avatar from UserDataFactory
         */
        this.getUserAvatar = function() {
            UserDataFactory.getAvatar($scope.nucleusId, getAvatarSize())
                .then(updateAvatarImage).catch(handleUpdateAvatarException);
        };

        /**
         * Get user avatar from API
         */
        function getUserAvatarFromServer() {
            UserDataFactory.getAvatarFromServer($scope.nucleusId, getAvatarSize())
                .then(updateAvatarImage).catch(handleUpdateAvatarException);
        }
        var updateUserAvatarThrottled = _.throttle(getUserAvatarFromServer, AVATAR_UPDATE_THROTTLE_MS); // Throttled version so we don't query too frequently

        // store the event handles
        handles = [
            AuthFactory.events.on('myauth:clientonlinestatechanged', onOnlineStateChanged),
            Origin.events.on(Origin.events.DIRTYBITS_AVATAR, updateUserAvatarThrottled)
        ];

        $scope.$on('$destroy', onDestroy);

    }

    function originAvatar(ComponentsConfigFactory) {

        function originAvatarLink(scope, element, attrs, ctrl) {
            ctrl.getUserAvatar();
            scope.onClick = angular.isDefined(attrs.clickOverride) ? scope.clickOverride : scope.goToProfile;
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
