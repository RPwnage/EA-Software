/**
 * @file store/about/scripts/nav.js
 */
(function () {
    'use strict';

    var EVENT_NAME = 'aboutAnchor:added';

    function originStoreAboutNavCtrl($scope, $location, AppCommFactory) {
        $scope.navClick = function (anchorName) {
            $location.hash(anchorName);
            AppCommFactory.events.fire('aboutNav:hashChange', anchorName);
        };
    }

    function originStoreAboutNav(ComponentsConfigFactory, QueueFactory) {

        function originStoreAboutNavLink(scope) {
            scope.navItems = [];

            function registerAnchor(anchor) {
                scope.navItems.push(anchor);
            }

            var unfollowQueue = QueueFactory.followQueue(EVENT_NAME, registerAnchor);

            scope.$on('$destroy', unfollowQueue);
        }

        return {
            restrict: 'E',
            scope: {},
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/about/views/nav.html'),
            link: originStoreAboutNavLink,
            controller: 'originStoreAboutNavCtrl',
            replace: true
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAboutNav
     * @restrict E
     * @element ANY
     * @scope
     * @description Creates a navigation menu for about page
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-about-nav></origin-store-about-nav>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('originStoreAboutNavCtrl', originStoreAboutNavCtrl)
        .directive('originStoreAboutNav', originStoreAboutNav);
}());
