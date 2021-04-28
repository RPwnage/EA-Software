/**
 * @file store/pdp/scripts/nav.js
 */
(function (){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-pdp-nav',
        ANCHORS_SELECTOR = '.l-store-pdp-navitem[show-on-nav="true"]';

    /**
     * The controller
     */
    function OriginStorePdpNavCtrl($scope, $location, PdpUrlFactory, ProductFactory, ObjectHelperFactory, UtilFactory, $document, AppCommFactory, ComponentsConfigFactory) {
        var takeHead = ObjectHelperFactory.takeHead;

        ProductFactory
            .filterBy(PdpUrlFactory.getCurrentEditionSelector())
            .addFilter(takeHead)
            .attachToScope($scope, 'model');

        $scope.strings = {
            overflowText: UtilFactory.getLocalizedStr($scope.overflowText, CONTEXT_NAMESPACE, 'overflowstr')
        };

        $scope.items = [];
        $scope.itemsMap = null;
        $scope.activeItemIndex = 0;
        $scope.pinned = false;

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

        AppCommFactory.events.on('navitem:added', function (newItem) {
            if (!!newItem) {
                $scope.$evalAsync(function () {
                    $scope.items.push(newItem);
                });
            }
        });
    }

    function originStorePdpNav($timeout, ComponentsConfigFactory, $window) {

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
                bannersHeight = navbar.height();

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
               return angular.element(window).scrollTop();
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
                            visibleAnchorName = scope.strings.overflowText;
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

                if (scope.pinned !== pinned) {
                    scope.pinned = pinned;
                    scope.$broadcast('pills:resize');
                }
            }

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
            angular.element($window).on('scroll', onScroll);
        }

        return {
            restrict: 'E',
            scope: {},
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
