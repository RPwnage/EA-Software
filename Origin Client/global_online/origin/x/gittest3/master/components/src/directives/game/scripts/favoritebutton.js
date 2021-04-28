/**
 * @file game/favoritebutton.js
 */
(function() {
    'use strict';

    function OriginGameFavoriteButtonCtrl($scope, UtilFactory, GamesDataFactory) {
        var CONTEXT_NAMESPACE = 'origin-game-favorite-button';

        function filtersReady() {
            return GamesDataFactory.isFavorite($scope.offerId);
        }

        $scope.tooltipLabelStr = UtilFactory.getLocalizedStr($scope.tooltiplabel, CONTEXT_NAMESPACE, 'tooltiplabel');

        this.isFavorited = function() {
            return GamesDataFactory.waitForFiltersReady()
                .then(filtersReady);
        };

        this.addFavoriteGame = function() {
            GamesDataFactory.addFavoriteGame($scope.offerId);
        };

        this.removeFavoriteGame = function() {
            GamesDataFactory.removeFavoriteGame($scope.offerId);
        };
    }

    function OriginGameFavoriteButtonLink(scope, element, attrs, ctrl) {
        var favoritedClassName = 'favorited';

        ctrl.isFavorited().then(function(isFavorited) {
            if (isFavorited) {
                element.addClass(favoritedClassName);
            }
        });

        scope.toggleFavorite = function() {
            if (element.hasClass(favoritedClassName)) {
                element.removeClass(favoritedClassName);
                ctrl.removeFavoriteGame();
            } else {
                element.addClass(favoritedClassName);
                ctrl.addFavoriteGame();
            }
        };
    }

    function originGameFavoriteButton(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                offerId: '@offerid',
                tooltiplabel: '@tooltiplabel',
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/views/favoritebutton.html'),
            controller: OriginGameFavoriteButtonCtrl,
            link: OriginGameFavoriteButtonLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameFavoriteButton
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} tooltiplabel the tooltip label for the button
     * @param {string} offerid the offer id to favorite
     * @description
     *
     * A standalone button to favorite a game
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-favorite-button offerid="OFB-EAST:109549060" tooltiplabel="Favorite"></origin-game-favorite-button>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGameFavoriteButtonCtrl', OriginGameFavoriteButtonCtrl)
        .directive('originGameFavoriteButton', originGameFavoriteButton);
}());