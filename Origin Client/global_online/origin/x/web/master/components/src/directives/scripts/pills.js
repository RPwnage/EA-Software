/**
 * @file pills.js
 * @author Preet Jassi
 */
(function() {
    'use strict';

    // constants
    var CLS_NGHIDE = 'ng-hide',
        CLS_NAV = 'origin-pills',
        CLS_PILLS = 'origin-pills-nav',
        CLS_PILLS_HIDDEN = 'origin-pills-nav-ishidden',
        CLS_PILL = 'otkpill',
        CLS_PILLS_BAR = 'otknav-pills-bar',
        CLS_PILL_OVERFLOW = 'otkpill-overflow',
        CLS_RESIZING = 'origin-pills-isresizing',

        OVERFLOW_MARGIN = 25;


    function originPills(ComponentsConfigFactory, $timeout, UtilFactory) {

        function originPillsLink(scope, element) {

            var body = angular.element(document.getElementsByTagName('body')[0]),
                nav = element[0].querySelector('.' + CLS_NAV),
                pillsList = nav.querySelector('.' + CLS_PILLS),
                bar = angular.element(element[0].querySelector('.' + CLS_PILLS_BAR)),
                overflow = element[0].querySelector('.' + CLS_PILL_OVERFLOW),
                overflowWidth = 0,
                overflowMargins = 0,
                firstTime = true,
                resizing = false;

            scope.showDropdown = false;
            scope.maxItems = scope.navItems.length;
            scope.hiddenNavItems = [];

            /**
            * Update the maximum number of pills to show and
            * optionally show the overflow
            * @method updateNav
            * @return {void}
            */
            var updateNav = UtilFactory.reverseThrottle(function() {
                var maxItems = scope.maxItems,
                    newMaxItems = setMaxPills();
                if (maxItems !== newMaxItems) {
                    updateHiddenItems();
                    if (scope.hiddenNavItems.length) {
                        showOverflow();
                    } else {
                        hideOverflow();
                    }
                    updateBarPosition();
                    scope.$digest();
                }
            }, 30);

            /**
            * Remove the resizing class name that sets the position
            * of the nav to absolute and sets overflow hidden so that
            * we do not have the nav drop down to the next level
            * when we are resizing
            * @method removeResizingClass
            * @return {void}
            */
            var removeResizingClass = UtilFactory.reverseThrottle(function() {
                resizing = false;
                nav.classList.remove(CLS_RESIZING);
                pillsList.style.width = 'auto';
            }, 500);

            /**
            * Default the links to hash if there is no href
            * @method defaultLinks
            * @return {void}
            */
            function defaultLinks() {
                for (var i = 0; i < scope.navItems.length; i++) {
                    if (!scope.navItems[i].href) {
                        scope.navItems[i].href = '#';
                    }
                }
            }

            /**
            * Get the maximum number of pills that we can show for the nav
            * and update scope.maxItems with that value
            * @method setMaxPills
            * @return void
            */
            function setMaxPills() {

                if (!scope.navItems.length || !pillsList.offsetWidth) {
                    return;
                }

                var navWidth = nav.offsetWidth,
                    overflowWidthAndMargins = overflowWidth + overflowMargins,
                    lastElementIndex = scope.navItems.length - 1,
                    width = 0,
                    lastItem;

                for (var i = 0; i < lastElementIndex; i++) {
                    width += scope.navItems[i].width + scope.navItems[i].margins;
                    if (width + overflowWidthAndMargins + OVERFLOW_MARGIN > navWidth) {
                        scope.maxItems = i;
                        return scope.maxItems;
                    }
                }

                // if you make it here, then you are on the last item
                // and there should be no overflow so don't consider it
                lastItem = scope.navItems[lastElementIndex];
                if (width + lastItem.width + lastItem.margins > navWidth) {
                    scope.maxItems = lastElementIndex;
                } else {
                    scope.maxItems = scope.navItems.length;
                }

                return scope.maxItems;

            }

            /**
            * Calculate the left position of a nav item (a pill)
            * @param {Number} index - the index of the navItem
            * @return {Number}
            * @method getPillLeft
            */
            function getPillLeft(index) {
                if (!index) {
                    return 0;
                }
                var previousWidth = scope.navItems[index-1].width,
                    previousLeft = scope.navItems[index-1].left,
                    previousMargins = scope.navItems[index-1].margins;
                return previousWidth + previousLeft + previousMargins;
            }

            /**
            * Calculate the pill widths and and left position and
            * cache it into the navItems object so that we can easily
            * update the bar to the correct navItem
            * @method calcPillWidths
            * @return {void}
            */
            function calcPillWidths() {
                var pills = pillsList.querySelectorAll('.' + CLS_PILL),
                    computedStyle;
                for (var i = 0, j = pills.length; i < j; i++) {
                    computedStyle = window.getComputedStyle(pills[i]);
                    if (pills[i].classList.contains(CLS_PILL_OVERFLOW)) {
                        overflowWidth = Math.round(parseFloat(computedStyle.width, 10));
                        overflowMargins = Math.round(parseFloat(computedStyle.marginLeft, 10) + parseFloat(computedStyle.marginRight, 10));
                    } else {
                        // we need these to be seperate (width and margins)
                        // so that we can have a proper pill bar width
                        scope.navItems[i].width = Math.round(parseFloat(computedStyle.width, 10));
                        scope.navItems[i].margins = Math.round(parseFloat(computedStyle.marginLeft, 10) + parseFloat(computedStyle.marginRight, 10));
                        scope.navItems[i].left = getPillLeft(i);
                    }
                }
            }

            /**
            * Store the nav items that we will not display into another
            * array that will be shown in the dropdown
            * @method updateHiddenItems
            * @return {void}
            */
            function updateHiddenItems() {
                scope.hiddenNavItems = scope.navItems.slice(scope.maxItems);
            }

            /**
            * Show the pills, we initially have them hidden with position absolute
            * to calculate the widths
            * @method showPills
            * @return {void}
            */
            function showPills() {
                pillsList.classList.remove(CLS_PILLS_HIDDEN);
            }

            /**
            * Hide the overflow when we don't need it,
            * initially we show the overflow to calculate the width
            * of the overflow container, but then after we initialize
            * we hide it.
            * @method hideOverflow
            * @return {void}
            */
            function hideOverflow() {
                overflow.classList.add(CLS_NGHIDE);
            }

            /**
            * Show the overflow container
            * @method showOverflow
            * @return {void}
            */
            function showOverflow() {
                overflow.classList.remove(CLS_NGHIDE);
            }

            /**
            * Hide the dropdown
            * @method hideDropdown
            * @return {void}
            */
            function hideDropdown() {
                scope.showDropdown = false;
                scope.$digest();
            }

            /**
            * Update the bar position to the selected index
            * @method updateBarPosition
            * @return {void}
            */
            function updateBarPosition() {
                var width, left;
                if (scope.selectedIndex >= scope.maxItems) {
                    width = overflowWidth;
                    left = scope.navItems[scope.maxItems].left;
                } else {
                    width = scope.navItems[scope.selectedIndex].width;
                    left = scope.navItems[scope.selectedIndex].left;
                }
                bar.css({
                    'width': width + 'px',
                    'transform': 'translate3d(' + left + 'px, 0, 0)',
                    '-webkit-transform': 'translate3d(' + left + 'px, 0, 0)'
                });
            }

            /**
            * Init the bar to the selected bar position or to the beginning
            * @method initBar
            * @return {void}
            */
            function initBar() {
                if (!scope.navItems.length) {
                    return;
                }
                if (angular.isUndefined(scope.selectedIndex)) {
                    scope.selectedIndex = 0;
                }
                updateBarPosition();
            }


            /**
            * On resize we update the nav, but we also
            * set the position of the nav to absolute and
            * overflow hidden so that we don't get any jankyness
            * when resizing down.
            * @method onResize
            * @return {void}
            */
            function onResize() {
                if (!resizing) {
                    resizing = true;
                    var last = scope.navItems[scope.navItems.length-1];
                    pillsList.style.width = (last.left + last.width + last.margins + OVERFLOW_MARGIN) + 'px';
                    nav.classList.add(CLS_RESIZING);
                }
                updateNav();
                removeResizingClass();
            }


            /**
            * Initialization, so first we calculate the pill widths and positions, then
            * we determine the maximum number of pills we can show, and then show that
            * maximum number of pills and show/hide the overflow
            * @method init
            * @return {void}
            */
            function init() {

                // we need to do this for IE - IE doesn't seem to
                // trigger the ng-class digest right away, even though
                // we are using a timeout (which is disgusiting btw)
                // so we need to put it in another timeout, so gross, i know
                if (!angular.isUndefined(scope.theme) && firstTime) {
                    firstTime = false;
                    setTimeout(init);
                    return;
                }

                defaultLinks();
                calcPillWidths();
                hideOverflow();
                setMaxPills();
                updateHiddenItems();
                if (scope.hiddenNavItems.length) {
                    showOverflow();
                } else {
                    hideOverflow();
                }
                showPills();
                initBar();
            }

            /**
            * Trigger a resize, but we have to use setTimeout to wait
            * for any reflows to occur properly.
            * @method resize
            * @return {void}
            */
            function resize() {
                setTimeout(onResize, 100);
            }

            /**
            * When you click on a navItem, either the integrator
            * is in charge of updating the selectedIndex, so we
            * trigger the integrators passed down callback function,
            * or we update the selectedIndex ourselves.
            * @param {Object} item - the navItem we clicked on
            * @param {Number} index - the index of the item we clicked on
            * @method scope.navItemClick
            * @return {void}
            */
            scope.navItemClick = function($event, item, index) {
                $event.preventDefault();
                if (scope.navClick) {
                    scope.navClick()(item);
                } else {
                    scope.selectedIndex = index;
                }
            };

            /**
            * When you change an item on the select dropdown,
            * trigger either the integrators callback or set the selected
            * index to the index of the select dropdown
            * @method scope.onChange
            * @return {void}
            */
            scope.onChange = function() {
                var index = parseInt(scope.navSelectValue, 10),
                    item = scope.navItems[index];
                if (scope.navClick) {
                    scope.navClick()(item);
                } else {
                    scope.selectedIndex = index;
                }
            };

            /**
            * Show / hide the dropdown when you click on the caret
            * and add an event when you click on the body to hide the
            * dropdown.
            * @method scope.toggleDropdown
            * @return {void}
            */
            scope.toggleDropdown = function() {
                scope.showDropdown = !scope.showDropdown;
                if (scope.showDropdown) {
                    setTimeout(function() {
                        body.one('click', hideDropdown);
                    }, 0);
                }
            };


            angular.element(window).on('resize', onResize);

            // expose on resize so that anything that
            // needs to trigger a resize can do it
            scope.$on('pills:resize', resize);

            // execute init next digest cycle because we need the repeat to finish
            // and the digest cycle to run.
            $timeout(init);

            scope.$watch('selectedIndex', updateBarPosition);

        }

        return {
            restrict: 'E',
            link: originPillsLink,
            scope: {
                navItems: '=',
                theme: '@',
                navClick: '&',
                selectedIndex: '='
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('views/pills.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originPills
     * @restrict E
     * @element ANY
     * @scope
     * @param {Array} navItems - an array of objects that look like {'anchorName': 'media', 'href': '#/store'}
     *  where anchorName is the label of the nav item, and href is optional and is a link
     *  to route that you want to navigate to.
     * @param {String} theme - optional, right now the only option is light, not including this parameter
     *  will default the nav to the default theme (dark).
     * @param {Function} navClick - a callback function to fire when you click on a nav item.  This callback
     *  will have to update the selected index if you specify it.
     * @param {Number} selectedIndex - the currently selected index
     * @description
     *
     * This component creates a pills navigation that automatically updates the overflow counter and
     * it defaults to a select dropdown on mobile resolutions.  In order to integrate you just need
     * to ass to the pills navigation an array of objects with the labels as the property anchorName.
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-pills theme='light' nav-items='items' nav-click='navClick' selected-index='activeItemIndex'></origin-pills>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .directive('originPills', originPills);
}());