/**
 * @file classmodifier.js
 */
(function () {
    'use strict';

    function originClassmodifier(AppCommFactory, EventsHelperFactory, UtilFactory, LayoutStatesFactory, AnimateFactory, ComponentsConfigHelper) {

        function originClassmodifierLink (scope) {
            /*
             * Create observable
             */
            LayoutStatesFactory
            .getObserver()
            .attachToScope(scope, 'states');

            /*
             * Initial setting of social class
             */
            scope.isOIGContext = ComponentsConfigHelper.isOIGContext();
            scope.isSocialEnabled = ComponentsConfigHelper.enableSocialFeatures();

            if(scope.isSocialEnabled) {
                scope.isSocialAccessiblityEnabled=true;
            }

            /*
             * Controls resize event callbacks
             */
            AnimateFactory.addResizeEventHandler(scope, LayoutStatesFactory.handleWindowResize, 300);
        }

        return {
            restrict: 'A',
            link: originClassmodifierLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originClassmodifier
     * @restrict A
     * @element ANY
     * @description Controls high-level class, gives app info about various UI states
     */
    angular.module('origin-components')
        .directive('originClassmodifier', originClassmodifier);
}());
