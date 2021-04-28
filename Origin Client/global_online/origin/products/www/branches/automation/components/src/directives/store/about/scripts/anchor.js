/**
 * @file store/about/scripts/anchor.js
 */
(function () {
    'use strict';

    /* jshint ignore:start */
    var AnchorIconEnumeration = {
        "otkicon-4k": "otkicon-4k",
        "otkicon-app": "otkicon-app",
        "otkicon-challenge": "otkicon-challenge",
        "otkicon-chatbubble": "otkicon-chatbubble",
        "otkicon-cloud": "otkicon-cloud",
        "otkicon-controller": "otkicon-controller",
        "otkicon-coop": "otkicon-coop",
        "otkicon-crossplatform": "otkicon-crossplatform",
        "otkicon-directx": "otkicon-directx",
        "otkicon-dolby": "otkicon-dolby",
        "otkicon-download": "otkicon-download",
        "otkicon-downloadnegative": "otkicon-downloadnegative",
        "otkicon-firstperson": "otkicon-firstperson",
        "otkicon-ggg": "otkicon-ggg",
        "otkicon-leaderboard": "otkicon-leaderboard",
        "otkicon-modding": "otkicon-modding",
        "otkicon-mouse": "otkicon-mouse",
        "otkicon-multiplayer": "otkicon-multiplayer",
        "otkicon-offlinemode": "otkicon-offlinemode",
        "otkicon-originlogo": "otkicon-originlogo",
        "otkicon-person": "otkicon-person",
        "otkicon-progressiveinstall": "otkicon-progressiveinstall",
        "otkicon-refresh": "otkicon-refresh",
        "otkicon-splitscreen": "otkicon-splitscreen",
        "otkicon-thirdperson": "otkicon-thirdperson",
        "otkicon-timer": "otkicon-timer",
        "otkicon-touch": "otkicon-touch",
        "otkicon-trophy": "otkicon-trophy",
        "otkicon-twitch": "otkicon-twitch"
    };
    /* jshint ignore:end */

    /**
     * The controller
     */
    function OriginStoreAboutAnchorCtrl($scope, $location, $timeout, AppCommFactory, ScreenFactory, QueueFactory) {

        var self = this;

        function getNavHeight() {
            return ScreenFactory.isSmall() ? angular.element('.l-origin-navigation').height() : 0;
        }

        /**
         * Scroll to id target element
         */
        function onHashChange() {
            var currentHash = $location.hash();
            if (currentHash === $scope.anchorName) {
                ScreenFactory.scrollTo(currentHash, getNavHeight(), 1000);
            }
        }

        /**
         * Register Anchor data with nav directive
         * @param event
         */
        self.registerAnchor = function (event) {
            QueueFactory.enqueue(event, {
                anchorName: $scope.anchorName,
                anchorIcon: $scope.anchorIcon,
                title: $scope.titleStr
            });
        };

        /**
         * Listen to hash change events
         */
        self.listenToHashChangeEvents = function () {
            // on manual URL hash-change, use custom scrolling
            AppCommFactory.events.on('aboutNav:hashChange', onHashChange);

            // attempt to scroll to this navitem when it is deep linked
            $timeout(self.onHashChange, 500);


            $scope.$on('$destroy', function () {
                AppCommFactory.events.off('aboutAnchor:hashChange', onHashChange);
            });
        };

    }

    function originStoreAboutAnchor(ComponentsConfigFactory) {

        function originStoreAboutAnchorLink(scope, element, attributes, originStoreAboutAnchorCtrl) {
            originStoreAboutAnchorCtrl.listenToHashChangeEvents();
            originStoreAboutAnchorCtrl.registerAnchor('aboutAnchor:added');
        }

        return {
            restrict: 'E',
            scope: {
                titleStr: '@',
                anchorName: '@',
                anchorIcon: '@'
            },
            controller: 'OriginStoreAboutAnchorCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/about/views/anchor.html'),
            link: originStoreAboutAnchorLink,
            replace: true
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAboutAnchor
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} title-str the title of the navigation item
     * @param {string} anchor-name the hash anchor name to link to
     * @param {AnchorIconEnumeration} anchor-icon icon for link (will appear in nav)
     * @description Creates a navigation point for deep-linking on a page, and emits its presence for inclusion on about nav.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-about-anchor title-str="Friends Who Play" anchor-name="friends" anchor-icon="otkicon-timer"></origin-store-about-anchor>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreAboutAnchorCtrl', OriginStoreAboutAnchorCtrl)
        .directive('originStoreAboutAnchor', originStoreAboutAnchor);
}());
