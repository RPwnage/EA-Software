/**
 * @file scripts/pagethinking.js
 */
(function () {
    'use strict';


    function originPagethinking(PageThinkingFactory, ComponentsConfigHelper) {

        function originPagethinkingLink(scope, element) {

            function handleChanges() {
                element.toggleClass('origin-ui-block', PageThinkingFactory.isBlocked());
                if (!!ComponentsConfigHelper.getOIGContextConfig()) {
                    element.toggleClass('origin-ui-block-oig', PageThinkingFactory.isBlocked());
                }
            }

            var unsubscribe = PageThinkingFactory.subscribeToChanges(handleChanges);

            scope.$on('$destroy', unsubscribe);
        }

        return {
            restrict: 'A',
            link: originPagethinkingLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originPagethinking
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     *
     * @example
     *
     */
    angular.module('origin-components')
        .directive('originPagethinking', originPagethinking);
}());
