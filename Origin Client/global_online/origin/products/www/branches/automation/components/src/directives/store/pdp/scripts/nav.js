/**
 * @file store/pdp/scripts/nav.js
 */
(function (){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-pdp-nav',
        NAV_ITEM_CLASS = '.l-store-pdp-navitem',
        ANCHORS_SELECTOR = NAV_ITEM_CLASS + '[show-on-nav="true"]';
    /**
     * The controller
     */
    function OriginStorePdpNavCtrl($scope, $location, AppCommFactory, QueueFactory, ComponentsConfigFactory) {


        $scope.items = [];
        $scope.itemsMap = null;
        $scope.activeItemIndex = 0;
        $scope.pinned = false;
        $scope.hasSocialHub = false;
        $scope.siteStripeMargin = 0;

        //incase packart is missing
        $scope.packArtDefault = ComponentsConfigFactory.getImagePath('packart-placeholder.jpg');

        function createItemsMap() {
            if (!$scope.itemsMap && $scope.items.length) {
                $scope.itemsMap = {};
                for (var i=0, j=$scope.items.length; i<j; i++) {
                    $scope.itemsMap[$scope.items[i].anchorName] = i;
                }
            }
        }

        $scope.goTo = function (anchorName) {
            $location.hash(anchorName);
            AppCommFactory.events.fire('navitem:hashChange', anchorName);
        };

        $scope.navClick = function(item) {
            createItemsMap();
            $scope.goTo(item.anchorName);
            $scope.activeItemIndex = $scope.itemsMap[item.anchorName];
        };

        /**
         * Reset the navigation status
         */
        $scope.reset = function () {
            $scope.scrollToTheTop();
            $scope.goTo('');
        };


        this.createItemsMap = createItemsMap;
        var navItems = [];
        var orderedNavItems = {};
        var navItemCount = 0;


        function setAnchorItems() {
            var finalArray = [];
            _.forEach(orderedNavItems, function(navItemValue) {
                if (navItemValue) {
                    finalArray.push(navItemValue);
                }
            });

            $scope.items = finalArray; //update the model
        }

        function addAnchorItem(anchorObject) {
            //if item is found and to be displayed on screen
            navItems.push({ //add it to navItems list
                anchorName: anchorObject.anchorName,
                title: anchorObject.anchorTitle,
                showOnNav: anchorObject.showOnNav === 'true'
            });
            navItemCount++;
        }

        function registerAnchor(anchorObject) {

            var totalNavItems = angular.element(NAV_ITEM_CLASS).length; //total number of nav items
            //index of nav item being registered
            addAnchorItem(anchorObject);

            _.forEach(navItems, function(navItemValue) {
                var navItemIndex = angular.element('#' + navItemValue.anchorName).index(NAV_ITEM_CLASS);
                if (navItemIndex >= 0) {
                    orderedNavItems[navItemIndex] = navItemValue.showOnNav ? navItemValue : null;
                }
            });

            if (navItemCount === totalNavItems) { //when all elements are registered
                setAnchorItems();
            }
        }

        var unfollowQueue = QueueFactory.followQueue('navitem:added', registerAnchor);

        $scope.$on('$destroy', unfollowQueue);
    }

    function originStorePdpNav(ComponentsConfigFactory, AnimateFactory, PdpUrlFactory, DirectiveScope, ScreenFactory, SocialHubFactory, EventsHelperFactory) {

        function createMenuItem(anchor) {
            var anchorElement = angular.element(anchor);

            return {
                anchorName: anchorElement.attr('anchor-name'),
                title: anchorElement.attr('title')
            };
        }

        function getAnchors() {
            return angular.element(ANCHORS_SELECTOR);
        }

        function originStorePdpNavLink(scope, elem, attrs, ctrl) {
            var navbar = elem.find('.store-pdp-nav-bar'),
                bannersHeight = navbar.height(),
                isClientBrowser = Origin.client.isEmbeddedBrowser();

            DirectiveScope.populateScopeWithOcdAndCatalog(scope, CONTEXT_NAMESPACE, PdpUrlFactory.buildPathFromUrl());

            /**
             * Populate nav bar with navitem elements
             */
            function populateItems() {
                var items = [];
                // loop through existing navitem elements
                angular.forEach(getAnchors(), function (e) {
                    items.push(createMenuItem(e));
                });
                scope.items = items;
            }

            /**
             * Returns the top position of the currently visible portion of the page
             */
            function getScrollTop() {
               return angular.element(window).scrollTop() + scope.siteStripeMargin;
            }

            /**
             * Gets the currently visible anchor (the one above or in the viewport).
             */
            function getVisibleAnchorName() {
                var visibleAnchorName = '',
                    scrollTop = getScrollTop() + navbar.outerHeight();

                getAnchors().each(function(index) {
                    var anchorElement = $(this),
                        anchorElementTop = anchorElement.offset().top - bannersHeight;

                    if (anchorElementTop <= scrollTop) {
                        visibleAnchorName = anchorElement.attr('anchor-name');

                        if (index >= scope.overflowIndex) {
                            visibleAnchorName = scope.overflowText;
                        }
                    }
                });
                return visibleAnchorName;
            }

            /**
             * Determine whether the active nav item is changed or not
             * based on the scroll position
             */
            function recalculateActiveItemIndex() {
                var anchorName = getVisibleAnchorName();
                ctrl.createItemsMap();
                scope.activeItemIndex = (scope.itemsMap && scope.itemsMap[anchorName]) || 0;
            }

            /**
             * Determine whether pdp-nav should be pinned to top
             */
            function recalculatePinnedStatus() {
                var scrollTop = getScrollTop(),
                    navPosition = elem.offset().top,
                    pinned = (navPosition < scrollTop);

                scope.hasSocialHub = isClientBrowser && ScreenFactory.isLarge() && !SocialHubFactory.isWindowPoppedOut() && !SocialHubFactory.isWindowMinimized();

                if (scope.pinned !== pinned) {
                    scope.pinned = pinned;
                    scope.$broadcast('pills:resize');
                }
            }

            /**
             * Listens to site stripe margins through origin-add-sitestripe-margin set in the html
             */
            scope.setSitestripeMargin = function(margin) {
                scope.siteStripeMargin = margin;
                updateNavbarMargin();
            };

            /**
             * If there is a site stripe and pinned add a margin
             */
            function updateNavbarMargin() {
                var navbar = elem.find('.store-pdp-nav-bar');
                if (navbar.length !== 0) {
                    navbar.css('margin-top', scope.pinned ? scope.siteStripeMargin : 0);
                }
            }

            scope.$watch('pinned', updateNavbarMargin);

            function onScroll() {
                recalculateActiveItemIndex();
                recalculatePinnedStatus();
                scope.$digest();
            }


            scope.scrollToTheTop = function() {
                angular.element('html, body').animate({ scrollTop: 0 }, 'fast');
                scope.activeItemIndex = 0;
            };

            // populate pdp nav with existing navitems and any late arrivals
            populateItems();

            // highlight nav-bar items if they're in view
            recalculateActiveItemIndex();

            // pin pdp nav to top when required
            AnimateFactory.addScrollEventHandler(scope, onScroll);


            function onSocialHubChange(){
                recalculatePinnedStatus();
                scope.$broadcast('pills:resize');
            }
            var handles = [
                SocialHubFactory.events.on('popOutHub', onSocialHubChange),
                SocialHubFactory.events.on('unpopOutHub', onSocialHubChange)
            ];
            EventsHelperFactory.attachAll(handles);

            function detachEvents(){
                EventsHelperFactory.detachAll(handles);
            }

            scope.$on('$destroy', detachEvents);
        }

        return {
            restrict: 'E',
            scope: {
                packArtOverride : '@'
            },
            controller: 'OriginStorePdpNavCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/nav.html'),
            link: originStorePdpNavLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpNav
     * @restrict E
     * @element ANY
     * @scope
     * @param {ImageUrl} pack-art-override pack art image for announcement/retire/no catalog packArt.
     * @param {LocalizedString} overflow-text overflow text
     * @description PDP navigation bar
     *
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-nav></origin-store-pdp-nav>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .controller('OriginStorePdpNavCtrl', OriginStorePdpNavCtrl)
        .directive('originStorePdpNav', originStorePdpNav);

}());
