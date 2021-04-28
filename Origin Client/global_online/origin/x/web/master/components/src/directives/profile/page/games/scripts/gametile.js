(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginProfilePageGamesGametileCtrl($scope, UtilFactory) {

        var CONTEXT_NAMESPACE = 'origin-profile-page-games-gametile';

        $scope.learnMoreLoc = UtilFactory.getLocalizedStr($scope.learnMoreStr, CONTEXT_NAMESPACE, 'learnmorestr');

        this.viewGameInfo = function () {
            window.alert('Clicked Game Tile Button');
        };

    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originProfilePageGamesGametile
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offerid the offer id
     * @param {string} titlestr the title of the game
     * @param {string} image the box art image URL
     * @param {LocalizedString} learnmorestr "Learn More"
     * @description
     *
     * Profile Page - Simple pack art game tile
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-profile-page-games-gametile
     *             imagestr="{imageURL}"
     *             title="{game name}"
     *             offerid="{OFFERID}"
     *             learnmorestr="Learn More"
     *         ></origin-profile-page-games-gametile>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginProfilePageGamesGametileCtrl', OriginProfilePageGamesGametileCtrl)
        .directive('originProfilePageGamesGametile', function (ComponentsConfigFactory) {

            return {
                restrict: 'E',
                controller: 'OriginProfilePageGamesGametileCtrl',
                scope: {
                    image: '@image',
                    titlestr: '@titlestr',
                    offerId: '@offerId',
                    learnMoreStr: '@learnmorestr'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('profile/page/games/views/gametile.html'),
                link: function (scope, element, attrs, ctrl) {
                    scope = scope;
                    attrs = attrs;
                    element = element;

                    // Listen for clicks on the info button
                    // or gametile (small screens only)
                    $(element).on('click', '.origin-profile-games-gametile-info-button, .origin-profile-games-gametile', function () {
                        ctrl.viewGameInfo();
                    });
                }

            };

        });
}());

