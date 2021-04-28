(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginProfilePageGamesSelfCtrl($scope, $timeout, GamesDataFactory, ObjectHelperFactory, ComponentsLogFactory, UtilFactory) {

        var CONTEXT_NAMESPACE = 'origin-profile-page-games-self';

        $scope.showMoreGamesLoc = UtilFactory.getLocalizedStr($scope.showMoreGamesStr, CONTEXT_NAMESPACE, 'showmoregamesstr');
        $scope.showLessGamesLoc = UtilFactory.getLocalizedStr($scope.showLessGamesStr, CONTEXT_NAMESPACE, 'showlessgamesstr');
        $scope.noGamesLoc = UtilFactory.getLocalizedStr($scope.noGamesStr, CONTEXT_NAMESPACE, 'nogamesstr');

        var getProperty = ObjectHelperFactory.getProperty;

        $scope.myGames = [];
        $scope.gamesInitialLimit = 10;
        $scope.gamesLimit = $scope.gamesInitialLimit;

        function parseGameList(list) {

            $.each(list, function (key, value) {
                $scope.myGames.push({'title': value.i18n.displayName, 'image': value.i18n.packArtLarge, 'offerId': value.offerId});
            });

            var gamesTotal = $scope.myGames.length;
            if (gamesTotal < $scope.gamesLimit) {
                $scope.gamesLimit = gamesTotal;
            }

            $scope.myGamesLoc = UtilFactory.getLocalizedStr($scope.myGamesStr, CONTEXT_NAMESPACE, 'mygamesstr')
                .replace('%number%', $scope.gamesLimit)
                .replace('%total%', gamesTotal);

            $scope.$digest();
        }

        function getOwnedOfferIds() {
            return GamesDataFactory.baseGameEntitlements().map(getProperty('offerId'));
        }

        function onUpdate() {

            $scope.myGames = [];

            var myOfferIds = getOwnedOfferIds();

            GamesDataFactory.getCatalogInfo(myOfferIds).then(parseGameList)
                .catch(function (error) {
                    ComponentsLogFactory.error('[origin-profile-page-games-self GamesDataFactory.onUpdate] error', error.message);
                });
        }


        $scope.$on('$destroy', function () {
            //disconnect listening for events
            GamesDataFactory.events.off('games:baseGameEntitlementsUpdate', onUpdate);
        });

        //setup a listener for any entitlement updates
        GamesDataFactory.events.on('games:baseGameEntitlementsUpdate', onUpdate, this);

        //if we already have data, lets use the entitlement info thats already there
        if (GamesDataFactory.initialRefreshCompleted()) {
            onUpdate();
        }

        this.showMoreGames = function () {
            var gamesTotal = $scope.myGames.length;
            $scope.gamesLimit = gamesTotal;
            $scope.myGamesLoc = UtilFactory.getLocalizedStr($scope.myGamesStr, CONTEXT_NAMESPACE, 'mygamesstr')
                .replace('%number%', $scope.gamesLimit)
                .replace('%total%', gamesTotal);
            $scope.$digest();
        };

        this.showLessGames = function () {
            var gamesTotal = $scope.myGames.length;
            $scope.gamesLimit = $scope.gamesInitialLimit;
            if (gamesTotal < $scope.gamesLimit) {
                $scope.gamesLimit = gamesTotal;
            }
            $scope.myGamesLoc = UtilFactory.getLocalizedStr($scope.myGamesStr, CONTEXT_NAMESPACE, 'mygamesstr')
                .replace('%number%', $scope.gamesLimit)
                .replace('%total%', gamesTotal);
            $scope.$digest();
        };

    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originProfilePageGamesSelf
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} mygamesstr "My Games - %number% of %total%"
     * @param {LocalizedString} nogamesstr "You don't have any games yet"
     * @param {LocalizedString} showmoregamesstr "Show More Games"
     * @param {LocalizedString} showlessgamesstr "Show Less Games"
     * @description
     *
     * Profile Page - Games - Self
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-profile-page-games-self
     *            yourgamesstr="My Games - %number% of %total%"
     *            showmoregamesstr="Show More Games"
     *            showlessgamesstr="Show Less Games"
     *            nogamesstr="You don't have any games yet"
     *         ></origin-profile-page-games-self>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginProfilePageGamesSelfCtrl', OriginProfilePageGamesSelfCtrl)
        .directive('originProfilePageGamesSelf', function (ComponentsConfigFactory) {

            return {
                restrict: 'E',
                controller: 'OriginProfilePageGamesSelfCtrl',
                scope: {
                    myGamesStr: '@mygamesstr',
                    showMoreGamesStr: '@showmoregamesstr',
                    showLessGamesStr: '@showlessgamesstr',
                    noGamesStr: '@nogamesstr'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('profile/page/games/views/self.html'),
                link: function (scope, element, attrs, ctrl) {
                scope = scope;
                attrs = attrs;

                // Listen for clicks on the "show more / show less" links
                $(element).on('click', '.show-more-games', function () {
                    ctrl.showMoreGames();
                });
                $(element).on('click', '.show-less-games', function () {
                    ctrl.showLessGames();
                });

            }

            };

        });
}());

