/**
 * Created by tirhodes on 2015-02-20.
 * @file section.js
 */
(function () {

    'use strict';

    /**
    * The controller
	*/
    function OriginSocialHubChatroomsSectionCtrl($scope, $filter, SocialDataFactory) {
        $scope.chatrooms = [];
        $scope.filteredChatroomsArray = [];

        var self = this;

        SocialDataFactory.getGroups().then(function(groups) {
            getChatroomsBySection(groups);
        }, function () {
        });

        function handleGroupListUpdate(groups) {
            getChatroomsBySection(groups);
        }

        function onDestroy() {
            SocialDataFactory.events.off('socialGroupListUpdated', handleGroupListUpdate);
        }

        SocialDataFactory.events.on('socialGroupListUpdated', handleGroupListUpdate);
        $scope.$on('$destroy', onDestroy);

        function getChatroomsBySection(groups) {

            $scope.chatrooms = [];

            $.each(groups, function (i, group) {
                if ($scope.sectionName === 'INVITATIONS' && group.inviteGroup) {
                    $scope.chatrooms.push(group);
                }
                else if ($scope.sectionName === 'CHATROOMS' && !group.inviteGroup) {
                    $scope.chatrooms.push(group);
                }
            });

            $scope.sectionLength = $scope.chatrooms.length;
            $scope.$digest();
        }

		function filterChatroomsArray() {
			if ($scope.nameFilter.length) {
				$scope.filteredChatroomsArray = $filter('filter')($scope.chatrooms, {'name': $scope.nameFilter});
			} else {
				$scope.filteredChatroomsArray = $scope.chatrooms;
			}
			$scope.sectionLength = $scope.filteredChatroomsArray.length;
		}

		$scope.$watch('nameFilter', function () {
            filterChatroomsArray();
        });

        this.openSection = function() {
            $scope.$sectionElement.find('.origin-social-hub-section-downarrow').removeClass('origin-social-hub-section-downarrow-rotate');
            $scope.$headerElement.addClass('header-visible');

            $scope.$viewportElement.show();
        };
        
        this.closeSection = function() {
            $scope.$sectionElement.find('.origin-social-hub-section-downarrow').addClass('origin-social-hub-section-downarrow-rotate');
            $scope.$headerElement.removeClass('header-visible');

            $scope.$viewportElement.hide();
        };

        this.toggleSectionVisible = function() {
            if ($scope.$viewportElement.is(':hidden')) {                
                self.openSection();
                localStorage.removeItem('origin-social-hub-section-CHATROOMSLIST-'+$scope.sectionName+'-closed-'+Origin.user.originId());               
            } else {
                self.closeSection();                    
                localStorage.setItem('origin-social-hub-section-CHATROOMSLIST-'+$scope.sectionName+'-closed-'+Origin.user.originId(), 'true');              
            }
            $scope.$digest();
        };

    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialHubChatroomsSection
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} titlestr the title string of the section
     * @param {string} section name of the section for internal purposes
     * @param {string} filter Filter string typed in by the user to filter the chatroom list
     * @description
     *
     * origin social hub -> chatrooms -> section
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-social-hub-chatrooms-section
     *            titlestr="CHAT ROOMS"
     *            section="CHATROOMS"
     *            filter="foo"
     *         ></origin-social-hub-chatrooms-section>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialHubChatroomsSectionCtrl', OriginSocialHubChatroomsSectionCtrl)
        .directive('originSocialHubChatroomsSection', function(ComponentsConfigFactory) {

            return {
                restrict: 'E',
                controller: 'OriginSocialHubChatroomsSectionCtrl',
                scope: {
                    'titleStr': '@titlestr',
                    'sectionName': '@section',
                    'nameFilter': '@filter'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/hub/chatrooms/views/section.html'),
                link: function(scope, element, attrs, ctrl) {
                    scope = scope;
                    attrs = attrs;

                    $(element).on('click', '.origin-social-hub-section-header', function() {
                        ctrl.toggleSectionVisible();
                    });

                    scope.$sectionElement = $(element).find('.origin-social-hub-section');                                      
                    scope.$headerElement = $(element).find('.origin-social-hub-section-header');                    
                    scope.$viewportElement = $(element).find('.origin-social-hub-section-viewport');

                    if (localStorage.getItem('origin-social-hub-section-CHATROOMSLIST-'+scope.filter+'-closed-'+Origin.user.originId())) {
                        ctrl.closeSection();
                    }
                } // link
            };

        });
}());

