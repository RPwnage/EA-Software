/**
 * @file keyeventtarget.js
 */

(function () {
    'use strict';

    /**
    * The controller
    */
    function OriginSocialHubKeyeventtargetrosteritemCtrl($scope) {
        $scope.hasHighlight = false;
        $scope.isContextmenuVisible = false;
        
        function getVisibleTargets () {
            return $('[keytarget]:visible');
        }

        this.getContextmenuState = function () {
            return $scope.isContextmenuVisible;
        };

        this.getPrevious = function (focused) {
            var previous = focused;
            var keyboardTargets = getVisibleTargets();
            var index = keyboardTargets.index(focused);
            
            if (index > 0) {
                previous = keyboardTargets.get(index-1);
            }
            return previous;
        };

        this.getNext = function (focused) {
            var next = focused;
            var keyboardTargets = getVisibleTargets();
            var index = keyboardTargets.index(focused);
            
            if (index < (keyboardTargets.length-1)) {
                next = keyboardTargets.get(index+1);
            }
            return next;
        };

    }

    /**
    * The directive
    */
    function OriginSocialHubKeyeventtargetrosteritem ($timeout, AppCommFactory, EventsHelperFactory) {
        
        /**
        * The link 
        */
        function OriginSocialHubKeyeventtargetrosteritemLink (scope, elem, attrs, ctrl) {
			var keyEventsAPI = ctrl[0]; // keyEvents.js controller
			var thisController = ctrl[1]; // this controller

            /* Capture Interactions
            * --------------------------
            */
            $(elem).on('click', function (e) {
                // Handles clicks inside AND outside of roster area
                // Will send each type to its own handler
                $(elem).focus();
                keyEventsAPI.registerEventHandlers(elem);
                keyEventsAPI.handleLeftClickRosterEvent(e);
            });

            $(elem).bind('contextmenu', function (e) {
                e.preventDefault();

                $(elem).focus();
                keyEventsAPI.registerEventHandlers(elem);
                keyEventsAPI.handleRightClickEvent();
            });

            $(elem).on('keydown', function (e) {
                e.preventDefault();
                return false;
            });

            $(elem).on('keyup', function (e) {
                if(scope.hasHighlight && !scope.isContextmenuVisible) {
					keyEventsAPI.registerEventHandlers(elem);					
					keyEventsAPI.handleKeyboardEvent(e, elem);
				}
            });

            

            /* Events captured 100% of time
            * -----------------------------
            */
			function hideContextMenuAndFocus() {
                if(scope.hasHighlight && scope.isContextmenuVisible) {
                    scope.isContextmenuVisible = false;

                    AppCommFactory.events.fire('social:updateContextmenuState');

                    $(elem).focus();
                }
			}

            AppCommFactory.events.on('social:keyupLeft', hideContextMenuAndFocus);
			AppCommFactory.events.on('social:keyupEscape', hideContextMenuAndFocus);

            AppCommFactory.events.on('social:setHighlight', function () {
                scope.hasHighlight = scope.isContextmenuVisible || keyEventsAPI.isFocused(elem);
            });



            /* Handle click events
            * --------------------------
            */
            AppCommFactory.events.on('social:rosterLeftClick', function (data) {
                // Hides context menu when clicking away
                // Makes exception for clicking on contextmenu icon
                var isIcon = $(data.eventTarget).hasClass('origin-social-hub-roster-contextmenu-otkicon-downarrow');

                if(keyEventsAPI.isFocused(elem)) {
                    
                    if(isIcon && !scope.isContextmenuVisible) {
                        // Inner friend directive uses this
                        scope.isContextmenuVisible = keyEventsAPI.isFocused(elem);

                        focusFirstContextMenuItem();
                    } else {
                        scope.isContextmenuVisible = false;
                    }

                } else {
                    scope.isContextmenuVisible = false;
                }
            
            });

            AppCommFactory.events.on('social:rosterRightClick', function () {
                if (keyEventsAPI.isFocused(elem)) {

					// Inner friend directive uses this
					scope.isContextmenuVisible = keyEventsAPI.isFocused(elem);

					focusFirstContextMenuItem();
				} else {
                    scope.isContextmenuVisible = false;
                }
            });



            /* Helpers
            * --------------------------
            */
			
			function focusFirstContextMenuItem() {
                
				$timeout(function() {
					var first = $(elem).find('[origin-social-hub-keyeventtargetcontextmenuitem][keytarget]:visible').first();
					$(first).focus();

					keyEventsAPI.registerEventHandlers(first);
				}, 0, false);
				
			}



            /* Event handler references
            * --------------------------
            */
            function handleSocialKeyupUp () {
                if(scope.hasHighlight) {
                    if(!scope.isContextmenuVisible) {
                    
                        var prev = thisController.getPrevious(elem);
                        scope.hasHighlight = false;
                        $(prev).focus();
                    }
                }
            }

            function handleSocialKeyupDown () {
                if(scope.hasHighlight) {
                    if(!scope.isContextmenuVisible) {
                        var next = thisController.getNext(elem);

                        scope.hasHighlight = false;
                        $(next).focus();

                        keyEventsAPI.registerEventHandlers(next);
                    }
                }
            }

            function handleSocialKeyupRight () {
                // Order of operations here matters.
                if(scope.hasHighlight) {

                    scope.isContextmenuVisible = true;

                    // Updates all item cmenu states
                    AppCommFactory.events.fire('social:updateContextmenuState');
					
					focusFirstContextMenuItem();
                }
            }

            function handleSocialKeyupEnter () {
                if(scope.hasHighlight) {
                    if(!scope.isContextmenuVisible) {

                        AppCommFactory.events.fire('social:triggerEnterEvent', { 'targetType': 'item', 'thisItem' : elem });
                    }
                }                
            }

            
            /* Handlers */
            var handles = [];

            // Attach events on interaction
            scope.attachHandlers = function () {
				if (!handles.length) {
					handles = [
						AppCommFactory.events.on('social:keyupUp', handleSocialKeyupUp),
						AppCommFactory.events.on('social:keyupDown', handleSocialKeyupDown),
						AppCommFactory.events.on('social:keyupRight', handleSocialKeyupRight),
						AppCommFactory.events.on('social:keyupEnter', handleSocialKeyupEnter)
					];					
				} else {
					EventsHelperFactory.attachAll(handles);
				}
            };

            scope.detachHandlers = function () {
                EventsHelperFactory.detachAll(handles);
            };

            /* Reset */
            AppCommFactory.events.on('social:resetRosterDefaults', function () {
                scope.hasHighlight = false;
                scope.isContextmenuVisible = false;
            });
            
        }

        return {
            restrict: 'A',
            priority: -1,
            scope: true,
            require: ['^originSocialHubKeyevents', 'originSocialHubKeyeventtargetrosteritem'],
            controller: 'OriginSocialHubKeyeventtargetrosteritemCtrl',
            link: OriginSocialHubKeyeventtargetrosteritemLink
        };
    }
    
    angular.module('origin-components')
        .controller('OriginSocialHubKeyeventtargetrosteritemCtrl', OriginSocialHubKeyeventtargetrosteritemCtrl)
        .directive('originSocialHubKeyeventtargetrosteritem', OriginSocialHubKeyeventtargetrosteritem);
}());