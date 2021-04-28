/**
 * @file game/violator.js
 */

(function() {
    'use strict';

    //var CONTEXT_NAMESPACE = 'origin-game-violator';

    function originGameViolator(ComponentsConfigFactory, GameViolatorFactory, ComponentsLogFactory, $compile, $timeout) {

        function originGameViolatorLink(scope, element) {
            scope.violate = false;

            /**
             * Get info to display the violator
             * @return {void}
             * @method getViolatorInfo
             */
            function getViolatorInfo() {
                //we set the violate flag which will trigger the ng-if binding
                scope.violate = GameViolatorFactory.hasViolator(scope.offerId);
                scope.$digest();

                return GameViolatorFactory.getViolatorContent(scope.offerId);
            }

            /**
             * Set the child directive
             * @return {void}
             * @method setViolatorContent
             */
            function setViolatorContent(violatorDirective) {
                if (scope.violate) {
                    //we need to give the binding one frame to update so that the ng-if is true and the class selector can be found
                    $timeout(function() {
                        var violatorCon = element.find('.origin-game-violator'),
                            violatorContent = $compile('<' + violatorDirective + '></' + violatorDirective + '>')(scope);
                        violatorCon.append(violatorContent);
                    }, 0, false);
                }

            }

            GameViolatorFactory.init()
                .then(getViolatorInfo)
                .then(setViolatorContent)
                .catch(function(error) {
                    ComponentsLogFactory.error('[origin-game-violator GameViolatorFactory.init] error', error.stack);
                });

        }

        return {
            restrict: 'E',
            scope: {
                offerId: '@offerid'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/views/violator.html'),
            link: originGameViolatorLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameViolator
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offerid the offerid of the game you want to interact with
     * @description
     *
     * game tile
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-violator offerid="OFB-EAST:109549060"></origin-game-violator>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .directive('originGameViolator', originGameViolator);
}());