/**
 * @file pills.js
 * @author Preet Jassi
 */
(function() {
    'use strict';

    // classes
    var CLS_NAV = '.origin-pills',
        CLS_NAV_LIGHT = 'origin-pills-light',
        CLS_LIST_LIGHT = 'otknav-pills-light',
        CLS_OPENINGDROPDOWN = 'origin-pills-isopeningdropdown',
        CLS_PILLS_HIDDEN = 'origin-pills-nav-ishidden',
        CLS_PILLS = '.origin-pills-nav',
        CLS_PILL = '.otkpill',
        CLS_PILL_LIGHT = 'otkpill-light',
        CLS_OVERFLOW = 'otkpill-overflow',
        CLS_PILLS_BAR = '.otknav-pills-bar',
        CLS_HIDE = 'ng-hide',


    // constants
        CONTEXT_NAMESPACE = 'origin-pills',
        OVERFLOW_PADDING = 10;

    function originPills(UtilFactory, CSSUtilFactory, AnimateFactory, ComponentsConfigFactory) {

        function originPillsLink(scope, element) {

            scope.strings = {
                moreText: UtilFactory.getLocalizedStr(scope.moreText, CONTEXT_NAMESPACE, 'more-text')
            };

            var body = angular.element(document.getElementsByTagName('body')[0]),
                elm = element[0],
                nav = elm.querySelector(CLS_NAV),
                list = elm.querySelector(CLS_PILLS),
                pills,
                overflow = {
                    elm: elm.querySelector('.' + CLS_OVERFLOW),
                    list: elm.querySelector('.' + CLS_OVERFLOW + ' .otkmenu-dropdown')
                },
                bar = angular.element(elm.querySelector(CLS_PILLS_BAR)),
                numVisiblePills = 0;

            scope.showDropdown = false;

            /* ------------------------------ */
            /*                                */
            /*             Helpers            */
            /*                                */
            /* ------------------------------ */

            /**
            * We need to trigger repaints on the
            * nav as well as the overflow so we get
            * the correct dimensions after the we
            * add a class to the pills
            *
            */
            function triggerRepaint() {
                nav.style.display = 'none';
                nav.offsetWidth; // jshint ignore:line
                nav.style.display = 'block';
                overflow.elm.style.display = 'none';
                overflow.elm.offsetWidth; // jshint ignore:line
                overflow.elm.style.display = 'block';
            }


            /**
            * Add the light theme if needed
            * @method themePills
            */
            function themePills() {
                if (scope.theme === 'light') {
                    nav.classList.add(CLS_NAV_LIGHT);
                    list.classList.add(CLS_LIST_LIGHT);
                    for (var i=0, j=pills.length; i<j; i++) {
                        pills[i].classList.add(CLS_PILL_LIGHT);
                    }
                    triggerRepaint();
                }
            }

            /**
            * Default the links to hash if there is no href
            * @method defaultLinks
            * @return {void}
            */
            function defaultLinks() {
                for (var i = 0, j = scope.items.length; i < j; i++) {
                    if (!scope.items[i].href) {
                        scope.items[i].href = '#';
                    }
                }
            }

            /**
            * Get the DOM elements for the things that we are
            * ng-repeating
            * @method getListElements
            */
            function getListElements() {
                pills = list.querySelectorAll(CLS_PILL);
                overflow.items = overflow.list.getElementsByTagName('li');
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
                var item = scope.items[index-1],
                    previousWidth = item.width,
                    previousLeft = item.left,
                    previousMargins = item.margins;
                return previousWidth + previousLeft + previousMargins;
            }

            /**
            * Calculate the pill widths and and left position and
            * cache it into the items object so that we can easily
            * update the bar to the correct navItem
            * @method calcPillWidths
            * @return {void}
            */
            function calcPillWidths() {
                var computedStyle, pillObj;
                for (var i = 0, j = pills.length; i < j; i++) {
                    computedStyle = window.getComputedStyle(pills[i]);

                    //Don't update width and margin if element is hidden
                    if (computedStyle.display === 'none'){
                        continue;
                    }
                    pillObj = pills[i].classList.contains(CLS_OVERFLOW) ? overflow : scope.items[i];
                    // we need these to be seperate (width and margins)
                    // so that we can have a proper pill bar width
                    pillObj.width = Math.round(pills[i].offsetWidth);
                    pillObj.margins = Math.round(parseFloat(computedStyle.marginLeft, 10) + parseFloat(computedStyle.marginRight, 10));
                    pillObj.widthAndMargin = pillObj.width + pillObj.margins;
                    pillObj.left = getPillLeft(i);
                    pillObj.right = pillObj.left + pillObj.widthAndMargin;
                }
            }

            /**
            * Get the maximum number of pills that we can show for the nav
            * and update numVisiblePills with that value
            * @method setNumVisiblePills
            * @return {number} number of visible pills or undefined if no items exist
            */
            function setNumVisiblePills() {

                if (!scope.items.length || !list.offsetWidth) {
                    return;
                }

                var navWidth = nav.offsetWidth,
                    overflowWidthAndMargins = overflow.widthAndMargin + OVERFLOW_PADDING,
                    lastIndex = scope.items.length - 1;

                for (var i = 0, j = scope.items.length; i < j; i++) {
                    // don't consider overflow on the last element
                    if (i === lastIndex) {
                        overflowWidthAndMargins = OVERFLOW_PADDING;
                    }
                    // check to see if this nav item will put us over the navWidth
                    // if so, we have our upper limit on the number of elements to show
                    if (scope.items[i].right + overflowWidthAndMargins > navWidth) {
                        numVisiblePills = i;
                        return numVisiblePills;
                    }
                }

                // if you get here you can use the maximum number of pills
                numVisiblePills = scope.items.length;
                return numVisiblePills;
            }

            /**
            * Remove hidden from all the pills
            * @method clearHiddenPills
            */
            function clearHiddenPills() {
                var hidden = list.querySelectorAll('.' + CLS_HIDE);
                for (var i = 0, j = hidden.length; i<j; i++) {
                    hidden[i].classList.remove(CLS_HIDE);
                }
            }

            /**
            * Hide the pills that should be hidden
            * @method hidePills
            */
            function hidePills() {
                var len = scope.items.length;
                for (var i = 0; i < len; i++) {
                    if (i < numVisiblePills) {
                        overflow.items[i].classList.add(CLS_HIDE);
                    } else {
                        pills[i].classList.add(CLS_HIDE);
                    }
                }
                // if we are displaying all of the pills, hide the overflow
                if (numVisiblePills === len) {
                    overflow.elm.classList.add(CLS_HIDE);
                }
            }

            /**
            * Show the nav after you calculate everything
            * @method showNav
            */
            function showNav() {
                list.classList.remove(CLS_PILLS_HIDDEN);
            }

            /**
            * Hide the dropdown
            * @method hideDropdown
            * @return {void}
            */
            function hideDropdown() {
                scope.showDropdown = false;
                nav.classList.remove(CLS_OPENINGDROPDOWN);
                body.off('click', hideDropdown);
                scope.$digest();
            }

            /**
            * Update the bar position to the selected index
            * @method updateBarPosition
            * @return {void}
            */
            function updateBarPosition() {
                // numVisiblePills is set to scope.items.length
                // Need to change the logic here so that our check for left is always valid
                var lastVisiblePill = numVisiblePills - 1;
                var width, left, cssObj;
                if (lastVisiblePill >= 0 && scope.selectedIndex > lastVisiblePill) {
                    width = overflow.width;
                    left = scope.items[lastVisiblePill].right;
                } else {
                    width = scope.items[scope.selectedIndex].width;
                    left = scope.items[scope.selectedIndex].left;
                }

                cssObj = CSSUtilFactory.addVendorPrefixes('transform', 'translate3d(' + left + 'px, 0, 0)');
                cssObj.width = width + 'px';

                bar.css(cssObj);
            }

            /**
            * Init the bar to the selected bar position or to the beginning
            * @method initBar
            * @return {void}
            */
            function initBar() {
                if (!scope.items.length) {
                    return;
                }
                if (angular.isUndefined(scope.selectedIndex)) {
                    scope.selectedIndex = 0;
                }
                // this will call updateBarPosition immediately
                scope.$watch('selectedIndex', updateBarPosition);
            }


            /* ------------------------------ */
            /*                                */
            /*       Events / Callbacks       */
            /*                                */
            /* ------------------------------ */


            /**
            * Determine if the number of pills to hide or
            * show has changed and if so then trigger an update
            */
            function onResize() {
                setNumVisiblePills();
                clearHiddenPills();
                hidePills();
                updateBarPosition();
                if (scope.showDropdown) {
                    hideDropdown();
                }
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
            * Show / hide the dropdown when you click on the caret
            * and add an event when you click on the body to hide the
            * dropdown.
            * @method scope.toggleDropdown
            * @return {void}
            */
            scope.toggleDropdown = function() {
                scope.showDropdown = !scope.showDropdown;
                if (scope.showDropdown) {
                    // we have overflow hidden by default to
                    // prevent any of the nav items from dropping
                    // to another line on resize, we only need to
                    // add overflow visible when we are toggling a dropdown
                    nav.classList.add(CLS_OPENINGDROPDOWN);
                    setTimeout(function() {
                        body.one('click', hideDropdown);
                    }, 0);
                } else {
                    nav.classList.remove(CLS_OPENINGDROPDOWN);
                }
            };

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
                    item = scope.items[index];
                if (scope.navClick) {
                    scope.navClick()(item);
                } else {
                    scope.selectedIndex = index;
                }
            };

            /**
            * Wait for the lists to complete rendering then when both of the
            * lists are done, resolve the promise.
            * @return {Promise}
            * @method renderList
            */
            function renderLists() {
                return new Promise(function(resolve) {
                    var postRepeatCount = 0;

                    /**
                    * Once both of the post repeats have completed
                    * we execute our initialization logic
                    * @param {Event} $event
                    * @method handlePostRepeat
                    */
                    function handlePostRepeat($event) {
                        $event.preventDefault();
                        $event.stopPropagation();
                        postRepeatCount++;
                        if (postRepeatCount === 2) {
                            resolve();
                        }
                    }

                    scope.$on('postrepeat:last', handlePostRepeat);

                });
            }

            scope.$on('pills:resize', resize);
            AnimateFactory.addResizeEventHandler(scope, onResize, 30);


            function onCollectionUpdate() {
                renderLists()
                    .then(defaultLinks)
                    .then(getListElements)
                    .then(themePills)
                    .then(calcPillWidths)
                    .then(setNumVisiblePills)
                    .then(clearHiddenPills)
                    .then(hidePills)
                    .then(showNav)
                    .then(initBar);
            }

            // initialize when the lists are rendered
            scope.$watchCollection('navItems', function (newItems) {
                onCollectionUpdate();
                // Need to set the items list (which is ng-repeat'ed) after we listen for postrepeat
                // otherwise its possible to miss the postrepeat event
                scope.items = angular.copy(newItems);
            });

        }

        return {
            restrict: 'E',
            link: originPillsLink,
            scope: {
                navItems: '=',
                theme: '@',
                navClick: '&',
                selectedIndex: '=',
                moreText: '@'
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
     * @param {Array} nav-items - an array of objects that look like {'anchorName': 'media', 'href': 'store'}
     *  where anchorName is the label of the nav item, and href is optional and is a link
     *  to route that you want to navigate to.
     * @param {String} theme - optional, right now the only option is light, not including this parameter
     *  will default the nav to the default theme (dark).
     * @param {Function} nav-click - a callback function to fire when you click on a nav item.  This callback
     *  will have to update the selected index if you specify it.
     * @param {Number} selected-index - the currently selected index
     * @param {LocalizedString} more-text label for nav menu when menu doesn't fit on screen
     * @description
     *
     * This component creates a pills navigation that automatically updates the overflow counter and
     * it defaults to a select dropdown on mobile resolutions.  In order to integrate you just need
     * to ass to the pills navigation an array of objects with the labels as the property anchorName.
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-pills theme='light' nav-items='items' nav-click='navClick' selected-index='activeItemIndex' more-text="More"></origin-pills>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .directive('originPills', originPills);
}());
