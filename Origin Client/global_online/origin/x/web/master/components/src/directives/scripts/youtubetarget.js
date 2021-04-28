
/**
 * @file /scripts/youtubetarget.js
 */
(function(){
    'use strict';
    function originYoutubetarget(YoutubeFactory) {
        function originYoutubetargetLink(scope, elem) {
            elem.addClass('youtube-video-target');
            var playerId = YoutubeFactory.createPlayerId();
            elem.attr('id', playerId);
        }
        return {
            restrict: 'A',
            link: originYoutubetargetLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originYoutubetarget
     * @restrict A
     * @element ANY
     * @scope
     * @description
     * @param {string=}
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-youtubetarget />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originYoutubetarget', originYoutubetarget);
}());
