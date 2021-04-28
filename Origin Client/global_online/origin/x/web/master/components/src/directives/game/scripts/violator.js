/**
 * @file game/violator.js
 */
(function() {

    'use strict';

    function originGameViolator($scope, ComponentsConfigFactory, GameViolatorFactory, ComponentsLogFactory, ObjectHelperFactory, OcdHelperFactory, GamesDataFactory, $compile) {

        function originGameViolatorLink(scope, element) {
            var takeHead = ObjectHelperFactory.takeHead,
                buildTag = OcdHelperFactory.buildTag;

            /**
             * Compile the directive for the game tile violator
             * @param {Object[]} violatorData the violator data
             */
            function showViolator(violatorData) {
                var tag ='',
                    violatorContainer = element.find('.l-origin-game-violator');

                if(!violatorData) {
                    return;
                }

                if(violatorData.violatorType === 'automatic') {
                    tag = buildTag('origin-game-violator-automatic', {
                            'violatortype': violatorData.label,
                            'enddate': violatorData.endDate,
                            'offerid': scope.offerId
                        });
                } else {
                    tag = buildTag('origin-game-violator-programmed', violatorData);
                }

                scope.violate = true;
                violatorContainer.append($compile(tag)(scope));

                scope.$digest();
            }

            function refresh() {
                scope.$digest();
            }

            function destruct() {
                GamesDataFactory.events.off('games:update:' + scope.offerId, refresh);
            }

            $scope.$on('$destroy', destruct);

            GamesDataFactory.events.on('games:update:' + scope.offerId, refresh);

            GameViolatorFactory.getViolators(scope.offerId, 'gametile')
                .then(takeHead)
                .then(showViolator)
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