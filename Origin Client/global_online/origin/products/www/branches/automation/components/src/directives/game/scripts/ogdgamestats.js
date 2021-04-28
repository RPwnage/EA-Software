/**
 * @file game/ogdgamestats.js
 */
(function() {
    'use strict';

    /**
     * LastPlayedDateTime display format
     * @see http://docs.adobe.com/docs/en/cq/5-6/widgets-api/index.html?class=Date
     */
    /* jshint ignore:start */
    var LastPlayedDateTime = {
        "format": "F m, Y"
    };
    /* jshint ignore:end */

    var CONTEXT_NAMESPACE = 'origin-game-ogd-gamestats',
        GAME_USAGE_TIMEOUT_MS = 800;

    function OriginGameOgdGamestatsCtrl($scope, $timeout, UtilFactory, GamesDataFactory, ComponentsLogFactory) {
        var getGameUsageTimeoutHandle;

        function secondsToHours (seconds) {
            return Math.floor(seconds / 60 / 60);
        }

        function handleGameUsageInfoError(error) {
            ComponentsLogFactory.error('[OGDGAMESTATS] getGameUsageInfo:', error);
        }

        function updateGameUsageInfo(data) {
            var hoursPlayed = 0;

            if (data.lastPlayedTime !== 0) {
                $scope.userLastPlayed = new Date(data.lastPlayedTime);
                $scope.userHasNotPlayed = undefined;
            } else {
                $scope.userHasNotPlayed = UtilFactory.getLocalizedStr($scope.userhasnotplayed, CONTEXT_NAMESPACE, 'userhasnotplayed');
                $scope.userLastPlayed = undefined;
            }

            if (data.totalTimePlayedSeconds > 0) {
                hoursPlayed = secondsToHours(data.totalTimePlayedSeconds);
                $scope.userNeverPlayed = undefined;
            } else {
                $scope.userNeverPlayed = UtilFactory.getLocalizedStr($scope.userneverplayed, CONTEXT_NAMESPACE, 'userneverplayed');
                $scope.userHoursPlayed = undefined;
            }

            // Show these all the time
            $scope.lastPlayedStr = UtilFactory.getLocalizedStr($scope.lastplayed, CONTEXT_NAMESPACE, 'lastplayed');
            $scope.hoursPlayedStr = UtilFactory.getLocalizedStr($scope.hoursplayed, CONTEXT_NAMESPACE, 'hoursplayed');
            // Only want to set Hours played if we have not defined userNeverPlayed already
            if (_.isUndefined($scope.userNeverPlayed)) {
                $scope.userHoursPlayed = UtilFactory.getLocalizedStr($scope.hoursplayedtemplate,
                        CONTEXT_NAMESPACE,
                        'hoursplayedtemplate',
                        {
                            '%hours%': hoursPlayed
                        },
                        hoursPlayed
                    );
            }

            $scope.$digest();
        }

        function onPlayTimeChanged(offerIds) {
            if (offerIds.indexOf($scope.offerId) >= 0) {
                requestGameUsageInfo();
            }
        }

        function requestGameUsageInfo() {
            getGameUsageTimeoutHandle = $timeout(getGameUsageInfo, GAME_USAGE_TIMEOUT_MS, false);
        }

        function getGameUsageInfo() {
            GamesDataFactory.getGameUsageInfo($scope.offerId)
                .then(updateGameUsageInfo)
                .catch(handleGameUsageInfoError);
        }

        function onDestroy() {
            Origin.events.off(Origin.events.CLIENT_GAMES_PLAYTIMECHANGED, onPlayTimeChanged);
            $timeout.cancel(getGameUsageTimeoutHandle);
        }

        Origin.events.on(Origin.events.CLIENT_GAMES_PLAYTIMECHANGED, onPlayTimeChanged);
        $scope.$on('$destroy', onDestroy);

        requestGameUsageInfo();
    }

    function originGameOgdGamestats(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerId: '@offerid',
                hoursplayed: '@',
                lastplayed: '@',
                hoursplayedtemplate: '@',
                userhasnotplayed: '@',
                userneverplayed: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/views/ogdgamestats.html'),
            controller: OriginGameOgdGamestatsCtrl
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameOgdGamestats
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} hoursplayed hours played heading text
     * @param {LocalizedString} lastplayed last played heading text
     * @param {LocalizedString} hoursplayedtemplate the pluralized templated for displaying the number of hours played
     * @param {LocalizedString} userhasnotplayed user has not recently played the game
     * @param {LocalizedString} userneverplayed user has never played this game
     * @param {string} offerid the offerid to use as a fallback
     * @description
     *
     * The ogd game stats header overlay - creates a definition list of game stats
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-ogd-gamestats
     *              offerid="OFB-EAST:109549060"
     *              hoursplayed="Hours Played"
     *              lastplayed="Last Played"
     *              hoursplayedtemplate = "{0} Under an Hour | {1} About 1 hour | ]1,+Inf] %hours% hours"
     *              userhasnotplayed = "Not played recently"
     *              userneverplayed = "Not played yet"
     *              >
     *        </origin-game-ogd-gamestats>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGameOgdGamestatsCtrl', OriginGameOgdGamestatsCtrl)
        .directive('originGameOgdGamestats', originGameOgdGamestats);
}());