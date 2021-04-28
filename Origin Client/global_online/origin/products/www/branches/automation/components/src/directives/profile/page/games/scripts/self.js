(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginProfilePageGamesSelfCtrl($scope, $timeout, GamesDataFactory, ObjectHelperFactory, ComponentsLogFactory, UtilFactory) {

        var CONTEXT_NAMESPACE = 'origin-profile-page-games-self',
            getProperty = ObjectHelperFactory.getProperty,
            transformWith = ObjectHelperFactory.transformWith,
            mapWith = ObjectHelperFactory.mapWith,
            toArray = ObjectHelperFactory.toArray,
            catalogDataMap = {
                'title': ['i18n', 'displayName'],
                'image': ['i18n', 'packArtLarge'],
                'offerId': 'offerId'
            },
            entitlementUpdateHandle = null;

        $scope.showMoreGamesLoc = UtilFactory.getLocalizedStr($scope.showMoreGamesStr, CONTEXT_NAMESPACE, 'showmoregamesstr');
        $scope.showLessGamesLoc = UtilFactory.getLocalizedStr($scope.showLessGamesStr, CONTEXT_NAMESPACE, 'showlessgamesstr');
        $scope.noGamesLoc = UtilFactory.getLocalizedStr($scope.noGamesStr, CONTEXT_NAMESPACE, 'nogamesstr');

        $scope.gamesInitialLimit = 10;
        $scope.gamesLimit = $scope.gamesInitialLimit;

        function processUpdate(games) {
            $scope.myGames = games;

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

        function handleError(error) {
            ComponentsLogFactory.error('[origin-profile-page-games-self GamesDataFactory.onUpdate] error', error);
        }

        function onUpdate() {
            var myOfferIds = getOwnedOfferIds();

            GamesDataFactory.getCatalogInfo(myOfferIds)
                .then(mapWith(transformWith(catalogDataMap)))
                .then(toArray)
                .then(processUpdate)
                .catch(handleError);
        }

        function onDestroy() {
            //disconnect listening for events
            if(entitlementUpdateHandle) {
                entitlementUpdateHandle.detach();
            }
        }

        $scope.$on('$destroy', onDestroy);

        //setup a listener for any entitlement updates
        entitlementUpdateHandle = GamesDataFactory.events.on('games:baseGameEntitlementsUpdate', onUpdate, this);

        //if we already have data, lets use the entitlement info thats already there
        if (GamesDataFactory.initialRefreshCompleted()) {
            onUpdate();
        }

        $scope.showMoreGames = function () {
            var gamesTotal = $scope.myGames.length;
            $scope.gamesLimit = gamesTotal;
            $scope.myGamesLoc = UtilFactory.getLocalizedStr($scope.myGamesStr, CONTEXT_NAMESPACE, 'mygamesstr')
                .replace('%number%', $scope.gamesLimit)
                .replace('%total%', gamesTotal);
            $scope.$digest();
        };

        $scope.showLessGames = function () {
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
                templateUrl: ComponentsConfigFactory.getTemplatePath('profile/page/games/views/self.html')
            };

        });
}());

