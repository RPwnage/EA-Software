/**
 * @file gamelibrary/ogd.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-gamelibrary-ogd',
        FLYOUT_BEGIN_ANIMATION_TIMEOUT_MS = 270,
        FLYOUT_SHOW_BACKGROUND_TIMEOUT_MS = 250;

    function OriginGamelibraryOgdCtrl($scope, $timeout, $element, AppCommFactory, UtilFactory, GamesDataFactory, $state) {
        var body = angular.element('body');
        var flyoutOgdHandle, fadeBackgroundHandle;
        $scope.offlineLoc = UtilFactory.getLocalizedStr($scope.offlineStr, CONTEXT_NAMESPACE, 'offlinestr');
        $scope.isNog = GamesDataFactory.isNonOriginGame($scope.offerId);
        $scope.topic = $state.params.topic;

        /**
         * Update the topic from a rebroadcasted uirouter event
         * @param  {Object} event    event
         * @param  {Object} toState  $state
         * @param  {Object} toParams $stateParams
         */
        function updateTopic(event, toState, toParams) {
            if (toParams.topic) {
                $scope.topic = toParams.topic;
                setTabIndex();
            }
        }

        /**
        * Set the tabIndex based on the current topic
        * @method setTabIndex
        */
        function setTabIndex() {
            var topics = _.pluck($scope.navItems, 'friendlyname'),
                index = topics.indexOf($scope.topic);
            $scope.tabIndex = index + 1 > 0 ? index : 0;
        }

        /**
         * Adds margin to stop the content from shifting while the OGD is open
         * modified from original jsfiddle
         * @see http://jsfiddle.net/ngwhk/
         */
        function bodyFreezeScroll() {
            var bodyWidth = body.innerWidth();

            body.css('overflow', 'hidden');
            body.css('marginRight', (body.css('marginRight') ? '+=' : '') + (body.innerWidth() - bodyWidth));
        }

        /**
         * Remove margin offset from the OGD
         * modified from original jsfiddle
         * @see http://jsfiddle.net/ngwhk/
         */
        function bodyUnfreezeScroll() {
            body.css('marginRight', '');
            body.css('overflow', 'auto');
        }

        /**
        * Initialization store the nav items
        * @param {Event} $event
        * @param {Array} data - the navItems
        * @method init
        */
        function init($event, data) {
            $event.stopPropagation();
            $scope.navItems = data;
            setTabIndex();
            initHandle(); // only init once

            $scope.$digest();
        }

        /**
        * Trigger a route change on clicking of the items
        * @method onTabChange
        */
        $scope.onTabChange = function(item) {
            var navItem = _.findWhere($scope.navItems, {'anchorName': item.anchorName});
            if (navItem) {
                AppCommFactory.events.fire('uiRouter:go', 'app.game_gamelibrary.ogd.topic', {
                    offerid: $scope.offerId,
                    topic: navItem.friendlyname
                });
            }
        };

        /**
         * Trigger the flyout once the doubleclick time has passed and preferably
         * after the background scrim is in place
         */
        function flyoutOgd() {
            bodyFreezeScroll();

            $element.find('.origin-ogd-content')
                .addClass('origin-ogd-content-to-front origin-ogd-content-expanded');
        }

        /**
         * Trigger the background scrim to draw. This should happen a little before
         * flying out as it causes a small delay on large screens
         */
        function fadeBackground() {
            $element.find('.origin-ogd-overlay')
                .addClass('origin-ogd-overlay-to-front origin-ogd-overlay-activated');
        }

        /**
        * Remove events
        * @method onDestroy
        */
        function onDestroy () {
            handle.detach();
            bodyUnfreezeScroll();
            body = null;
            $timeout.cancel(flyoutOgdHandle);
            $timeout.cancel(fadeBackgroundHandle);
        }

        var handle = AppCommFactory.events.on('uiRouter:stateChangeStart', updateTopic);
        var initHandle = $scope.$on('ogd:tabsready', init);
        $scope.$on('$destroy', onDestroy);

        flyoutOgdHandle = $timeout(flyoutOgd, FLYOUT_BEGIN_ANIMATION_TIMEOUT_MS, false);
        fadeBackgroundHandle = $timeout(fadeBackground, FLYOUT_SHOW_BACKGROUND_TIMEOUT_MS, false);
    }

    function originGamelibraryOgd(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerId: '@offerid',
                offlineStr: '@offlinestr'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/ogd.html'),
            controller: OriginGamelibraryOgdCtrl
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryOgd
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offerid the offer id
     * @param {LocalizedString} offlinestr "Game Details are not available in offline mode. Go Online to reconnect Origin."
     * @description
     *
     * owned game details container
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-gamelibrary-ogd offerid="{{offerId}}">
     *              <origin-gamelibrary-ogd-header
     *                  offerid="{{offerId}}"
     *                  offlinestr="Game Details are not available in offline mode. Go Online to reconnect Origin."
     *              ></origin-gamelibrary-ogd-header>
     *          </origin-gamelibrary-ogd>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .controller('OriginGamelibraryOgdCtrl', OriginGamelibraryOgdCtrl)
        .directive('originGamelibraryOgd', originGamelibraryOgd);
}());
