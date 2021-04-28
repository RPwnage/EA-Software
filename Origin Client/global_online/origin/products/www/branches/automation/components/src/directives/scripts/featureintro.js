/**
 * @file scripts/featureintro.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-featureintro';

    function originFeatureintro(ComponentsConfigFactory, FeatureIntroFactory, EventsHelperFactory, UtilFactory) {

        function originFeatureintroLink (scope, elem, attrs) {

            /* TODO: We need to redo this directive in future, we can not have context_namespace dynamically created
               it causes issues with strings which needs merchandizing */

            var featureName = angular.lowercase(attrs.feature),
                handles;

            scope.isVisible = false;

            function showCallout() {
                scope.isVisible = true;
            }

            scope.dismiss = function () {
                scope.isVisible = false;
            };

            handles = [
                FeatureIntroFactory.events.on('featureintro:show' + featureName, showCallout)
            ];

            /* Get Translated Strings */
            scope.strings = {
                'titleStr': UtilFactory.getLocalizedStr(scope.titleStr, CONTEXT_NAMESPACE, 'title-str'),
                'descStr': UtilFactory.getLocalizedStr(scope.descStr, CONTEXT_NAMESPACE, 'desc-str'),
                'btnStr': UtilFactory.getLocalizedStr(scope.btnStr, CONTEXT_NAMESPACE, 'btn-str')
            };

            /**
             * Unhook from factory events when directive is destroyed.
             */
            function onDestroy() {
                EventsHelperFactory.detachAll(handles);
            }

            scope.$on('$destroy', onDestroy);

        }

        return {
            restrict: 'E',
            scope: {
                descStr: '@',
                titleStr: '@',
                btnStr: '@'
            },
            link: originFeatureintroLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('views/featureintro.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originFeatureintro
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} title-str Got It!
     * @param {LocalizedString} desc-str You can view it again by opening the library filter and selecting 'Hidden games'.
     * @param {LocalizedString} btn-str You’ve hidden a game from your library
     *
     * @description
     *
     * Handles the visibility of the feature intro directive
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-featureintro feature="hideGame"></origin-featureintro>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .directive('originFeatureintro', originFeatureintro);
}());
