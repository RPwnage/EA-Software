/**
 * @file game/violator.js
 */
(function() {

    'use strict';

    var
        VIOLATOR_CONTAINER_ELEMENT_CLASSNAME = '.l-origin-game-violator',
        INLINE_VIOLATOR_COUNT = 1,
        VIOLATOR_CONTEXT = 'gametile';

    function originGameViolator(ComponentsConfigFactory, ViolatorObserverFactory, $compile, ScopeHelper, UtilFactory) {

        function originGameViolatorLink(scope, element) {
            var messages;

            /**
             * Given new model information clear and redraw the new dom elements
             * @param  {Object} model the violator model data
             */
            function updateViolatorDomFromModel(model) {
                var violatorContainer = element.find(VIOLATOR_CONTAINER_ELEMENT_CLASSNAME);

                if(!model) {
                    scope.violate = false;
                    violatorContainer.empty();
                    ScopeHelper.digestIfDigestNotInProgress(scope);
                    return;
                }

                scope.violate = true;
                violatorContainer
                    .empty()
                    .append($compile(model.inlineMessages)(scope));

                ScopeHelper.digestIfDigestNotInProgress(scope);
            }

            ViolatorObserverFactory.getObserver(scope.offerId, VIOLATOR_CONTEXT, INLINE_VIOLATOR_COUNT, messages, scope, UtilFactory.unwrapPromiseAndCall(updateViolatorDomFromModel));
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