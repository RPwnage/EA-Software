/**
 * Created by tirhodes on 2015-02-20.
 * @file listselector.js
 */
(function () {

    'use strict';
    
    /**
    * The controller
    */
    function OriginSocialHubListSelectorCtrl($scope, RosterDataFactory, UtilFactory) {
        
        var CONTEXT_NAMESPACE = 'origin-social-hub-listselector';

        $scope.friendsLoc = UtilFactory.getLocalizedStr($scope.friendsStr, CONTEXT_NAMESPACE, 'friendsstr');
        $scope.chatroomsLoc = UtilFactory.getLocalizedStr($scope.chatroomsStr, CONTEXT_NAMESPACE, 'chatroomsstr');
        
        this.activateTab = function(tab) {  
			// clear out current active tab
            var $oldTab = $('.origin-social-hub-listselector-tab.otktab-active');
			var oldTabTarget = $oldTab.attr('data-target');
            $oldTab.removeClass('otktab-active');
            
            // activate clicked tab            
            var $activeTab = $(tab).parents('.otktab');
            $activeTab.addClass('otktab-active');

			// Fade out old tab
            $('.'+oldTabTarget).fadeOut(250);

			// set the current active tab
			var tabTarget = $activeTab.attr('data-target');
			
			// Fade in new tab
			$('.'+tabTarget).fadeIn(250);
			
			// Update viewport on new tab
			if (tabTarget === 'origin-social-hub-roster-tab') {
				RosterDataFactory.updateRosterViewport();
			}
			
		};
		
    }
    
    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialHubListselector
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} friendsstr "Friends"
     * @param {LocalizedString} chatroomsstr "Chat Rooms"
     * @description
     *
     * origin social hub -> listselector
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-social-hub-listselector
     *            friendsstr="Friends"
     *            chatroomsstr="Chat Rooms"
     *         ></origin-social-hub-listselector>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialHubListSelectorCtrl', OriginSocialHubListSelectorCtrl)
        .directive('originSocialHubListselector', function(ComponentsConfigFactory) {
        
            return {
                restrict: 'E',
                controller: 'OriginSocialHubListSelectorCtrl',
                scope: {
                    friendsStr: '@friendsstr',
                    chatroomsStr: '@chatroomsstr'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/hub/views/listselector.html'),
                link: function(scope, element, attrs, ctrl) {                    
                    scope = scope;
                    attrs = attrs;
                    element = element;
                    ctrl = ctrl;

                    $(element).on('click', '.otktab', function(event) {
                        ctrl.activateTab(event.target);
                    });
                }
                
            };
        
        });
}());

