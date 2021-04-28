/**
 * @file game/favoritebutton.js
 */
(function() {
    'use strict';

    function OriginGameFavoriteButtonCtrl($scope, UtilFactory, GamesDataFactory) {
        var CONTEXT_NAMESPACE = 'origin-game-favorite-button';
        var self = this;

        function filtersReady() {
            return GamesDataFactory.isFavorite($scope.offerId);
        }

        function updateFavorites() {
            $scope.isFavorite = self.isFavorited();
            $scope.$digest();
        }

        $scope.tooltipLabelStr = UtilFactory.getLocalizedStr($scope.tooltiplabel, CONTEXT_NAMESPACE, 'tooltiplabel');

        self.isFavorited = function() {
            return GamesDataFactory.waitForFiltersReady()
                .then(filtersReady);
        };

        self.addFavoriteGame = function() {
            GamesDataFactory.addFavoriteGame($scope.offerId);
        };

        self.removeFavoriteGame = function() {
            GamesDataFactory.removeFavoriteGame($scope.offerId);
        };

        GamesDataFactory.events.on('filterChanged', updateFavorites);
        $scope.$on('$destroy', function() {
            GamesDataFactory.events.off('filterChanged', updateFavorites);
        });
    }

    function OriginGameFavoriteButtonLink(scope, element, attrs, ctrl) {
        var favoritedClassName = 'favorited';

        function updateFavorites() {
            ctrl.isFavorited().then(function(isFavorited) {
                if (isFavorited) {
                    element.addClass(favoritedClassName);
                } else {
                    element.removeClass(favoritedClassName);
                }
            });
        }

        scope.toggleFavorite = function() {
            if (element.hasClass(favoritedClassName)) {
                element.removeClass(favoritedClassName);
                ctrl.removeFavoriteGame();
            } else {
                element.addClass(favoritedClassName);
                ctrl.addFavoriteGame();
            }
        };

        updateFavorites();
        scope.$watch('isFavorite', updateFavorites);
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