/**
 * @file game/friendsplaying/icon.js
 */
(function() {
    'use strict';

    /**
     * Friends Playing Game directive
     * @directive originGameFriendsplayingIcon
     */
    function originGameFriendsplayingIcon(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                friendsPlaying: '=friendsplaying'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/friendsplaying/views/icon.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameFriendsplayingIcon
     * @restrict E
     * @element ANY
     * @scope
     * @param {Array} friendsplaying the array containing a list of friends
     *
     * friends playing icon
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-friendsplaying-icon friendsplaying="[]"></origin-game-friendsplaying-icon>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originGameFriendsplayingIcon', originGameFriendsplayingIcon);
}());