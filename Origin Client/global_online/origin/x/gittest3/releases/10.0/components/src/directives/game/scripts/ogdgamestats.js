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

    function OriginGameOgdGamestatsCtrl($scope, UtilFactory, GamesDataFactory, ComponentsLogFactory) {
        var CONTEXT_NAMESPACE = 'origin-game-ogd-gamestats';

        function handleGameUsageInfoError(error) {
            ComponentsLogFactory.error('[OGDGAMESTATS] getGameUsageInfo:',error.message);
        }

        function updateGameUsageInfo(response) {
            if (response) {
                $scope.lastPlayedStr = UtilFactory.getLocalizedStr($scope.lastplayed, CONTEXT_NAMESPACE, 'lastplayed');

                if (response.lastPlayedTime !== 0) {
                    $scope.userLastPlayed = new Date(response.lastPlayedTime);
                } else {
                    $scope.userHasNotPlayed = UtilFactory.getLocalizedStr($scope.userhasnotplayed, CONTEXT_NAMESPACE, 'userhasnotplayed');
                }

                if (response.totalTimePlayedSeconds !== 0) {
                    var hoursPlayed = Math.floor(response.totalTimePlayedSeconds / 60 / 60);
                    $scope.hoursPlayedStr = UtilFactory.getLocalizedStr($scope.hoursplayed, CONTEXT_NAMESPACE, 'hoursplayed');
                    $scope.userHoursPlayed = UtilFactory.getLocalizedStr(
                        $scope.hoursplayedtemplate,
                        CONTEXT_NAMESPACE,
                        'hoursplayedtemplate', {
                            '%hours%': hoursPlayed
                        },
                        hoursPlayed
                    );
                }
                else {
                    $scope.hoursPlayedStr = '';
                }

                $scope.$digest();
            }
        }

        GamesDataFactory.getGameUsageInfo($scope.offerId)
            .then(updateGameUsageInfo)
            .catch(handleGameUsageInfoError);
    }

    function originGameOgdGamestats(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerId: '@offerid',
                hoursplayed: '@hoursplayed',
                lastplayed: '@lastplayed',
                hoursplayedtemplate: '@hoursplayedtemplate',
                userhasnotplayed: '@userhasnotplayed'
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
     * @param {LocalizedString} userhasnotplayed user has not played the game yet
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