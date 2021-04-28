/**
 * @file gamelibrary/ogd.js
 */
(function() {
    'use strict';

    function OriginGamelibraryOgdCtrl() {
    }

    function originGamelibraryOgd(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerId: '@offerid'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/ogd.html'),
            controller: OriginGamelibraryOgdCtrl
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryOgd
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offerid the offer id
     * @description
     *
     * owned game details container
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-gamelibrary-ogd offerid="{{offerId}}">
     *              <origin-gamelibrary-ogd-header offerid="{{offerId}}"></origin-gamelibrary-ogd-header>
     *          </origin-gamelibrary-ogd>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .controller('OriginGamelibraryOgdCtrl', OriginGamelibraryOgdCtrl)
        .directive('originGamelibraryOgd', originGamelibraryOgd);
}());