/**
 * @file home/getstarted/tileconfig.js
 */
(function() {
    'use strict';

    var GetStartedTileEnumeration = {
        "getStartedAchievementVisibility": "getStartedAchievementVisibility",
        "getStartedCustomAvatar": "getStartedCustomAvatar",
        "getStartedOriginEmail": "getStartedOriginEmail",
        "getStartedProfileVisibility": "getStartedProfileVisibility",
        "getStartedRealName": "getStartedRealName",
        "getStartedTwofactorAuth": "getStartedTwofactorAuth",
        "getStartedVerifyEmail": "getStartedVerifyEmail"
    };

    function OriginHomeGetStartedTileconfigCtrl($scope, GetStartedFactory) {
        var tileConfig = {
            'name': GetStartedTileEnumeration[$scope.tileName],
            'priority': $scope.priority
        };
        GetStartedFactory.events.fire("getstarted:tileAdded", tileConfig);
    }

    function originHomeGetStartedTileconfig() {

        return {
            restrict: 'E',
            scope: {
                tileName: '@',
                priority: '@'
            },
            controller: OriginHomeGetStartedTileconfigCtrl
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeGetStartedTileconfig
     * @restrict E
     * @element ANY
     * @scope
     * @param {GetStartedTileEnumeration} tile-name Name of the tile to be displayed
     * @param {Number} priority Numeric priority level for this tile (larger numbers come first)
     * @description
     *
     * Config for Get Started tile to make tile appear and have certain prioritization
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *        <origin-home-get-started-tileconfig tile-name="getStartedCustomAvatar" priority="50"></origin-home-get-started-tileconfig>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originHomeGetStartedTileconfig', originHomeGetStartedTileconfig);
}());
