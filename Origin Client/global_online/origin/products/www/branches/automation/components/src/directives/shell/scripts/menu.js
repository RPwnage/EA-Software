/**
 * @file shell/scripts/menu.js
 */
(function() {
    'use strict';

    var CLS_NAV = 'origin-navigation',
        CLS_NAV_TOP = 'origin-navigation-top',
        CLS_NAV_PRIMARYMENU = 'origin-navigation-primarymenu',
        CLS_NAV_BOTTOM = 'origin-navigation-bottom',
        CLS_NAV_OVERFLOWING = 'origin-navigation-isoverflowing',
        CLS_NAV_TERTIARY = 'origin-navigation-tertiary',
        CLS_NAV_SCROLLABLE = 'origin-navigation-scrollable-container',
        CLS_NAV_ISMOBILE = 'origin-navigation-ismobile';

    function OriginShellMenuCtrl($scope) {
        $scope.mainMenuVisible = false;
    }

    function originShellMenu(ComponentsConfigFactory, UtilFactory, AuthFactory, AppCommFactory, $timeout, LayoutStatesFactory, EventsHelperFactory, $rootScope, ScreenFactory) {

        var isMobileView = ScreenFactory.isSmall(),
            mediaQueryRemoveListener,
            overflowing = false,
            currentScrollContainer = null;

        function originShellMenuLink(scope, elem) {

            var win = angular.element(window),
                nav = elem[0].querySelector('.' + CLS_NAV),
                navTop, navMain, navBottom, navTertiary, navTopHeight, navBottomHeight, navScrollable,
                throttledCheckOverflow, handles, navClosedProfilePadding;

            function calculateNavBottomHeight() {
                if (!navBottom) {
                    var bottoms = document.querySelectorAll('.' + CLS_NAV_BOTTOM);
                    for (var i=0, j=bottoms.length; i<j; i++) {
                        if (!bottoms[i].classList.contains('ng-hide')) {
                            navBottom = bottoms[i];
                            navClosedProfilePadding = navBottom.offsetHeight;
                            break;
                        }
                    }
                }

                navBottomHeight = navBottom && navBottom.offsetHeight ? navBottom.offsetHeight : 0;
            }

            function calculateNavTopHeight() {
                if(navTop) {
                    var cs = window.getComputedStyle(navTop);
                    navTopHeight = navTop.offsetHeight + navMain.offsetHeight + navTertiary.offsetHeight + parseInt(cs.marginTop, 10);                    
                }
            }

            function calculatePadding(paddingValue) {
                nav.style.paddingBottom = paddingValue + 'px';                    
            }

            function resetPadding() {
               calculatePadding(navClosedProfilePadding);
            }

            function cacheDomReferences() {
                navTop = nav.querySelector('.' + CLS_NAV_TOP);
                navMain = nav.querySelector('.' + CLS_NAV_PRIMARYMENU);
                navTertiary = nav.querySelector('.' + CLS_NAV_TERTIARY);
                navScrollable = nav.querySelector('.' + CLS_NAV_SCROLLABLE);
            }

            function checkOverflow() {
                //for backwards compatability, to handle the case if the navScrollable is falsy due to the directive
                //not being in CQ or the scrollable items are not wrapped.
                var navScrollContainer = navScrollable ? navScrollable : nav;
                if (nav.offsetHeight < (navTopHeight + navBottomHeight)) {
                    overflowing = true;
                    navScrollContainer.classList.add(CLS_NAV_OVERFLOWING);
                } else {
                    overflowing = false;
                    navScrollContainer.classList.remove(CLS_NAV_OVERFLOWING);
                }
            }

            function onHeightChange() {
                calculateNavTopHeight();
                calculateNavBottomHeight();
                calculatePadding(navBottomHeight);
                checkOverflow();
            }

            function hideStaticShell() {
                // we need to hide the staticshell and show the nav coming from CMS. This helps stop the flickering between the two.
                $rootScope.hideStaticShell = true;
            }

            function onMouseWheel(event) {
                if (currentScrollContainer) {
                    //we intercept the mousewheel to check we we can scroll any more on the nav, we cannot, eat the input so that other areas do not scroll
                    var scrollingUp = event.originalEvent.deltaY < 0,
                        atBottom = (currentScrollContainer.offsetHeight + currentScrollContainer.scrollTop === currentScrollContainer.scrollHeight),
                        atTop = currentScrollContainer.scrollTop === 0,
                        cannotScroll = (atBottom && !scrollingUp) || (atTop && scrollingUp);

                    if (cannotScroll && overflowing) {
                        return event.preventDefault();
                    }

                }
            }

            function onDestroy() {
                EventsHelperFactory.detachAll(handles);
                win.off('resize', throttledCheckOverflow);
                if(currentScrollContainer) {
                    angular.element(currentScrollContainer).unbind('wheel', onMouseWheel);
                }

                if(mediaQueryRemoveListener) {
                    mediaQueryRemoveListener();
                }

            }

            function clearNavBottomAndUpdateWindowCalculations() {
                navBottom = null;
                //we calculate the heights on the next frame, so that the offsetHeights will be correct
                //at the time init is called, the digest cycle hasn't happened to switch from static nav to true nav
                setTimeout(onHeightChange, 0);                
            }


            function updateNavLayoutInfo(mq) {
                var navScrollContainer = navScrollable ? navScrollable : nav;
                
                //if we have a media query already, we should just use that
                isMobileView = mq ? mq.matches : ScreenFactory.isSmall();

                //unhook the current mouse wheel listener (does nothing if listener was not previous hooked up)
                if(currentScrollContainer) {
                    angular.element(currentScrollContainer).unbind('wheel', onMouseWheel);
                }

                //update the current scroll container, we use the full nav if its mobile view, otherwise we just use the div containing
                //the top/primiary/teritary menus (navScrollable)
                currentScrollContainer =  isMobileView ? nav : navScrollable;

                //update all the height calculations
                setTimeout(onHeightChange, 0);   

                //hook up a new mouse wheel listener
                if(currentScrollContainer) {
                    angular.element(currentScrollContainer).bind('wheel', onMouseWheel);
                }

                //this is used in determining whether to hide the submenus or not 
                if (isMobileView) {
                    navScrollContainer.classList.add(CLS_NAV_ISMOBILE);
                } else {
                    navScrollContainer.classList.remove(CLS_NAV_ISMOBILE);
                }          
                
                //TODO: Remove this. there is a lag with the margin top disappearing/appearing when resizing from mobile to desktop mode
                //We shouldn't have to set the style here. There's a noticeable lag with the sitestripe margin logic when
                //transitioning from mobile to not mobile, that is too risky to to touch right now.
                if(navTop) {
                    if(isMobileView) {
                        navTop.style.marginTop = elem[0].style.marginTop;
                    } else {
                        navTop.style.marginTop = 0;                    
                    }
                }
            }

            function init() {
                hideStaticShell();
                cacheDomReferences();
                updateNavLayoutInfo();                
                handles = [
                    AppCommFactory.events.on('menuitem:clicked', scope.toggleMainMenu),
                    AppCommFactory.events.on('globalsitestripe:update', onHeightChange),
                    AppCommFactory.events.on('globalsitestripe:add', onHeightChange),
                    AppCommFactory.events.on('globalsitestripe:remove', onHeightChange),
                    AuthFactory.events.on('myauth:change', clearNavBottomAndUpdateWindowCalculations)
                ];
                scope.$digest();
            }

            /*
            * Create observable
            */
            LayoutStatesFactory
                .getObserver()
                .attachToScope(scope, 'states');

            /**
             * Display the sidenav when in mobile
             * @return {void}
             * @method
             */
             scope.toggleMainMenu = function(e) {
                if (e) {
                    e.preventDefault();
                }
                LayoutStatesFactory.handleMenuClick();
            };

            throttledCheckOverflow = UtilFactory.reverseThrottle(checkOverflow, 30);

            // listen to events

            // this timeout is so that we wait for all of the children to be loaded
            // and available, otherwise it may throw a JS error
            $timeout(init, 1000, false);
            win.on('resize', throttledCheckOverflow);
            scope.$on('navigation:heightchange', onHeightChange);
            scope.$on('$destroy', onDestroy);
            scope.$on('navigation:footermenuclosestart', resetPadding);            
            ScreenFactory.addSmallMediaQueryListener(updateNavLayoutInfo);
        }

        return {
            restrict: 'E',
            replace: true,
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('shell/views/menu.html'),
            controller: 'OriginShellMenuCtrl',
            link: originShellMenuLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originShellMenu
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * main menu
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-shell-menu></origin-shell-menu>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginShellMenuCtrl', OriginShellMenuCtrl)
        .directive('originShellMenu', originShellMenu);
}());
