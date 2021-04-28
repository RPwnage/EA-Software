/**
 * @file gamestate.js
 */

(function() {
    'use strict';

    function OriginAutomationGamestateCtrl($scope, COMPONENTSCONFIG, GamesDataFactory) {
        /**
         * get the game state object and convert it to a string
         * @return {string} the stringified version of the object
         */
        this.buildGameStateStringFromObject = function() {
            var game = GamesDataFactory.getClientGame($scope.offerId);
            return game ? JSON.stringify(game) : '';
        };

    }

    function originAutomationGamestate(COMPONENTSCONFIG, GamesDataFactory) {
        /**
         * link function for directive
         * @param  {object} scope   the scope of the directive
         * @param  {object} element the element of the directive
         */
        function originAutomationGamestateLink(scope, element, attrs, ctrl) {
            /**
             * update the game client state
             * @return {[type]} [description]
             */
            function updateGameClientState() {
                element.attr('data-gamestate', ctrl.buildGameStateStringFromObject());
            }

            /**
             * clean up listeners
             */
            function onDestroy() {
                GamesDataFactory.events.off('games:update:' + scope.offerId, updateGameClientState);
            }

            /**
             * run when controller initializes
             */
            function init() {
                if (COMPONENTSCONFIG.overrides.automation) {
                    updateGameClientState();
                    GamesDataFactory.events.on('games:update:' + scope.offerId, updateGameClientState);
                    scope.$on('$destroy', onDestroy);
                }
            }

            init();
        }
        return {
            restrict: 'A',
            controller: 'OriginAutomationGamestateCtrl',
            link: originAutomationGamestateLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originAutomationGamestate
     * @restrict A
     * @element ANY
     * @scope
     * @description
     *
     * Adds game state information as data attributes to the element
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <div origin-automation-gamestate></div>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginAutomationGamestateCtrl', OriginAutomationGamestateCtrl)
        .directive('originAutomationGamestate', originAutomationGamestate);

}());