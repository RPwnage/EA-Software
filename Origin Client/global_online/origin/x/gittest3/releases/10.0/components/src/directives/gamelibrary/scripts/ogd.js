/**
 * @file gamelibrary/ogd.js
 */
(function() {
    'use strict';

    function originGamelibraryOgd(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/ogd.html'),
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryOgd
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * owned game details container
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-gamelibrary-ogd>
     *              <origin-gamelibrary-ogd-header offerid="{{offerId}}"></origin-gamelibrary-ogd-header>
     *          </origin-gamelibrary-ogd>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .directive('originGamelibraryOgd', originGamelibraryOgd);
}());