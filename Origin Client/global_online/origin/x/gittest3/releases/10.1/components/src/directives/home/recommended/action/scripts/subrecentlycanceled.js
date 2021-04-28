/**
 * @file home/recommended/Subrecentlycanceled.js
 */
(function() {
    'use strict';

    function OriginHomeRecommendedActionSubrecentlycanceledCtrl($scope, LocFactory) {
        $scope.offerBased = false;
        $scope.ctaText = LocFactory.trans('Reactivate');
    }

    function originHomeRecommendedActionSubrecentlycanceled(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                image: '@image',
                title: '@title',
                subtitle: '@subtitle',
                description: '@description'
            },
            controller: 'OriginHomeRecommendedActionSubrecentlycanceledCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/recommended/action/views/tile.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeRecommendedActionSubrecentlycanceled
     * @restrict E
     * @element ANY
     * @scope
     * @param {ImageUrl} image image of the tile
     * @param {LocalizedString|OCD} title title on the tile
     * @param {LocalizedString} subtitle subtile on the tile
     * @param {LocalizedString} description description on the tile
     * @description
     *
     * sub recently canceled recommended next action
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-recommended-action-subrecentlycanceled image="https://qa.www.assets.cms.origin.com/content/dam/originx/web/app/home/actions/tile_chinarising_long.png" titlestr="Just Added! Battlefield 4 Premium" subtitlestr="Get Back In The Game!" descriptionstr="Renew your subscription now to get access to more than 30 great games.">
     *         </origin-home-recommended-action-subrecentlycanceled>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginHomeRecommendedActionSubrecentlycanceledCtrl', OriginHomeRecommendedActionSubrecentlycanceledCtrl)
        .directive('originHomeRecommendedActionSubrecentlycanceled', originHomeRecommendedActionSubrecentlycanceled);
}());