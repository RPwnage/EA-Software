(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginProfileTabsCtrl($scope, $stateParams, UtilFactory, AppCommFactory, NavigationFactory) {

        var CONTEXT_NAMESPACE = 'origin-profile-tabs';

        $scope.achievementsLoc = UtilFactory.getLocalizedStr($scope.achievementsStr, CONTEXT_NAMESPACE, 'achievementsstr');
        $scope.friendsLoc = UtilFactory.getLocalizedStr($scope.friendsStr, CONTEXT_NAMESPACE, 'friendsstr');
        $scope.gamesLoc = UtilFactory.getLocalizedStr($scope.gamesStr, CONTEXT_NAMESPACE, 'gamesstr');
        $scope.wishlistLoc = UtilFactory.getLocalizedStr($scope.wishlistStr, CONTEXT_NAMESPACE, 'wishliststr');

        $scope.isSelf = ($scope.nucleusId === '') || (Number($scope.nucleusId) === Number(Origin.user.userPid()));
        $scope.topic = $stateParams.topic;

        // Build the nav array for origin-pills directive
        $scope.navItems = [
            {
                anchorName: $scope.achievementsLoc,
                title: $scope.achievementsLoc,
                friendlyname: 'achievements'
            },
            {
                anchorName: $scope.friendsLoc,
                title: $scope.friendsLoc,
                friendlyname: 'friends'
            },
            {
                anchorName: $scope.gamesLoc,
                title: $scope.gamesLoc,
                friendlyname: 'games'
            },
            {
                anchorName: $scope.wishlistLoc,
                title: $scope.wishlistLoc,
                friendlyname: 'wishlist'
            }
        ];

        /**
        * Set the activeItemIndex based on the current topic
        * @method setActiveItemIndex
        */
        function setActiveItemIndex() {
            var topics = _.pluck($scope.navItems, 'friendlyname'),
                index = topics.indexOf($scope.topic);
            $scope.activeItemIndex = index + 1 > 0 ? index : 0;
        }

        /**
         * Update the topic from a rebroadcasted uirouter event
         * @param  {Object} event    event
         * @param  {Object} toState  $state
         * @param  {Object} toParams $stateParams
         */
        function updateTopic(event, toState, toParams) {
            if (toParams.topic) {
                $scope.topic = toParams.topic;
                setActiveItemIndex();
            }
        }

        /**
        * Trigger a route change on clicking of the items
        * @method onTabChange
        */
        $scope.onTabChange = function(item) {
            if ($scope.isSelf) {
                NavigationFactory.goToState('app.profile.topic', {
                    topic: item.friendlyname
                });
            } else {
                NavigationFactory.goUserProfile($scope.nucleusId, item.friendlyname);
            }
        };

        /**
        * Remove events
        * @method onDestroy
        */
        function onDestroy () {
            AppCommFactory.events.off('uiRouter:stateChangeStart', updateTopic);
        }

        AppCommFactory.events.on('uiRouter:stateChangeStart', updateTopic);
        $scope.$on('$destroy', onDestroy);

        // we need to set the active item index on page load or page refresh
        setActiveItemIndex();

    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originProfileTabs
     * @restrict E
     * @element ANY
     * @scope
     * @param {number} nucleusid - nucleusId of the user
     * @param {string} topic - the profile tab name - achievements, friends, or games
     * @param {LocalizedString} achievementsstr - "ACHIEVEMENTS"
     * @param {LocalizedString} friendsstr  "FRIENDS"
     * @param {LocalizedString} gamesstr  "GAMES"
     * @param {LocalizedString} wishliststr  "WISHLIST"
     * @description
     *
     * Profile Tabs
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-profile-tabs nucleusid="123456789"
     *            topic="achievements"
     *            achievementsstr="ACHIEVEMENTS"
     *            friendsstr="FRIENDS"
     *            gamesstr="GAMES"
     *            wishliststr="WISHLIST"
     *         ></origin-profile-tabs>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginProfileTabsCtrl', OriginProfileTabsCtrl)
        .directive('originProfileTabs', function (ComponentsConfigFactory) {

            return {
                restrict: 'E',
                controller: 'OriginProfileTabsCtrl',
                scope: {
                    nucleusId: '@nucleusid',
                    topic: '@',
                    achievementsStr: '@achievementsstr',
                    friendsStr: '@friendsstr',
                    gamesStr: '@gamesstr',
                    wishlistStr: '@wishliststr'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('profile/views/tabs.html')
            };
        });
}());
