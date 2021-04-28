(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginSocialInviteusersUserCtrl($scope, $timeout, UserDataFactory, RosterDataFactory, ObserverFactory, ComponentsLogFactory, ComponentsConfigFactory) {

        $scope.selected = false;
        $scope.avatarImgSrc = ComponentsConfigFactory.getImagePath('social//avatar_placeholder.png');

		function requestAvatar() {
			UserDataFactory.getAvatar($scope.user.nucleusId, Origin.defines.avatarSizes.SMALL)
				.then(function(response) {
					$scope.avatarImgSrc = response;
					$scope.$digest();
				}, function () {

				}).catch(function(error) {
				    ComponentsLogFactory.error('OriginSocialInviteusersUserCtrl: UserDataFactory.getAvatar failed', error);
				});
		}

		requestAvatar();

		RosterDataFactory.getRosterUser($scope.user.nucleusId).then(function (user) {
		    if (!!user) {
		        ObserverFactory.create(user._presence)
                    .defaultTo({ jid: $scope.user.nucleusId, presenceType: '', show: 'UNAVAILABLE', gameActivity: {} })
                    .attachToScope($scope, 'presence')
                    .attachToScope($scope, onPresenceUpdate);
		    }
		});

		function onPresenceUpdate() {
		    // If the user goes offline, remove them from the list
		    if ($scope.presence.show === 'UNAVAILABLE') {
		        $scope.parentController.removeUser($scope.user.nucleusId);
		    }
		}

		$scope.onClicked = function () {

		    if ($scope.selected) {
		        // Zoom out the checked icon
		        $scope.$iconSelected.removeClass('zoomIn');
		        $scope.$iconSelected.removeClass('zoomOut');
		        $timeout(function () {
		            $scope.$iconSelected.addClass('zoomOut');
		        }, 0);

		        // Wait for the animation to finish
		        $timeout(function () {

		            // Zoom in the unchecked icon
		            $scope.$iconSelect.removeClass('zoomIn');
		            $scope.$iconSelect.removeClass('zoomOut');
		            $timeout(function () {
		                $scope.$iconSelect.addClass('zoomIn');
		                $scope.selected = false;
		                $scope.parentController.removeUser($scope.user.nucleusId);
		            }, 0);

		        }, 125);
		    }
            else {
                // Zoom out the unchecked icon
		        $scope.$iconSelect.removeClass('zoomIn');
		        $scope.$iconSelect.removeClass('zoomOut');
		        $timeout(function () {
		            $scope.$iconSelect.addClass('zoomOut');
		        }, 0);

                // Wait for the animation to finish
		        $timeout(function () {

		            // Zoom in the checked icon
		            $scope.$iconSelected.removeClass('zoomIn');
		            $scope.$iconSelected.removeClass('zoomOut');
		            $timeout(function () {
		                $scope.$iconSelected.addClass('zoomIn');
		                $scope.selected = true;
		                $scope.parentController.addUser($scope.user.nucleusId);
		            }, 0);

		        }, 125);
            }

		};

		this.onLink = function () {
		    $scope.selected = $scope.parentController.isSelected($scope.user.nucleusId);
		};
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originInviteusersUser
     * @restrict E
     * @element ANY
     * @scope
     * @param {Object} user Javascript object representing the user for this user item
     * @description
     *
     * origin invite users user
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-social-inviteusers-user
     *            user="user"
     *         ></origin-social-inviteusers-user>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialInviteusersUserCtrl', OriginSocialInviteusersUserCtrl)
        .directive('originSocialInviteusersUser', function (ComponentsConfigFactory) {

            return {
                restrict: 'E',
                require: ['^originDialogContentInviteusers', 'originSocialInviteusersUser'],
                controller: 'OriginSocialInviteusersUserCtrl',
                scope: {
                    'user': '='
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/inviteusers/views/user.html'),
                link: function (scope, element, attrs, ctrl) {

                    element = element;
                    attrs = attrs;

                    scope.$iconSelect = $(element).find('.otkicon-add');
                    scope.$iconSelected = $(element).find('.otkicon-checkcircle');

                    scope.parentController = ctrl[0];
                    scope.thisController = ctrl[1];

                    scope.thisController.onLink();

                }
            };

        });

}());
