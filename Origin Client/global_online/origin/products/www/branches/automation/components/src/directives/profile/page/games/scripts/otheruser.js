(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginProfilePageGamesOtheruserCtrl($scope, GamesDataFactory, ComponentsLogFactory, ObjectHelperFactory, UtilFactory, RosterDataFactory,
        ComponentsConfigFactory, ObserverFactory, AuthFactory, CommonGamesFactory) {

        var CONTEXT_NAMESPACE = 'origin-profile-page-games-otheruser',
            entitlementUpdateHandle = null;

        $scope.showMoreGamesLoc = UtilFactory.getLocalizedStr($scope.showMoreGamesStr, CONTEXT_NAMESPACE, 'showmoregamesstr');
        $scope.showLessGamesLoc = UtilFactory.getLocalizedStr($scope.showLessGamesStr, CONTEXT_NAMESPACE, 'showlessgamesstr');
        $scope.privateLoc = UtilFactory.getLocalizedStr($scope.privateStr, CONTEXT_NAMESPACE, 'privatestr');
        $scope.noGamesLoc = UtilFactory.getLocalizedStr($scope.noGamesStr, CONTEXT_NAMESPACE, 'nogamesstr');

        $scope.privacy = {isPrivate: false};
        $scope.loading = true;
        $scope.commonGames = [];
        $scope.uncommonGames = [];
        $scope.commonGamesInitialLimit = 5;
        $scope.uncommonGamesInitialLimit = 10;
        $scope.commonGamesLimit = $scope.commonGamesInitialLimit;
        $scope.uncommonGamesLimit = $scope.uncommonGamesInitialLimit;

        RosterDataFactory.getRosterUser($scope.nucleusId).then(function(user) {
            if (!!user) {
                ObserverFactory.create(user.privacy)
                    .defaultTo({isPrivate: false})
                    .attachToScope($scope, 'privacy');
            }
        });

        function processGamesOwned(offerIds, commonOfferIds) {
            $scope.commonGames = [];
            $scope.uncommonGames = [];

            offerIds.forEach(function (element, index) {
                var imageUrl;
                index = index;
                if (!!element.packArtLarge) {
                    imageUrl = 'https://eaassets-a.akamaihd.net/origin-com-store-final-assets-prod' + element.packArtLarge;
                }
                else {
                    // No box art, use placeholder image
                    imageUrl = ComponentsConfigFactory.getImagePath('profile\\game-placeholder.jpg');
                }
                if (element.masterTitleId && commonOfferIds.hasOwnProperty(element.masterTitleId)) {
                    $scope.commonGames.push({ 'title': element.displayProductName, 'offerId': element.productId, 'image': imageUrl });
                }
                else {
                    $scope.uncommonGames.push({ 'title': element.displayProductName, 'offerId': element.productId, 'image': imageUrl });
                }
            });

            var commonGamesTotal = $scope.commonGames.length;
            var uncommonGamesTotal = $scope.uncommonGames.length;

            if (commonGamesTotal < $scope.commonGamesLimit) {
                $scope.commonGamesLimit = commonGamesTotal;
            }
            if (uncommonGamesTotal < $scope.uncommonGamesLimit) {
                $scope.uncommonGamesLimit = uncommonGamesTotal;
            }

            $scope.gamesYouBothOwnLoc = UtilFactory.getLocalizedStr($scope.gamesYouBothOwnStr, CONTEXT_NAMESPACE, 'gamesyoubothownstr')
                .replace('%number%', $scope.commonGamesLimit)
                .replace('%total%', commonGamesTotal);
            $scope.gamesYouDontOwnLoc = UtilFactory.getLocalizedStr($scope.gamesYouDontOwnStr, CONTEXT_NAMESPACE, 'gamesyoudontownstr')
                .replace('%number%', $scope.uncommonGamesLimit)
                .replace('%total%', uncommonGamesTotal);
            $scope.otherGamesLoc = UtilFactory.getLocalizedStr($scope.otherGamesStr, CONTEXT_NAMESPACE, 'othergamesstr')
                .replace('%number%', $scope.uncommonGamesLimit)
                .replace('%total%', uncommonGamesTotal);

            RosterDataFactory.setProfilePrivacy($scope.nucleusId, false);
            $scope.loading = false;
            $scope.$digest();
        }

        /**
         * saves offerIds and retrieves a list of catalogInfos for all offers
         * @param  {string[]} offerIds the list of offers to retrieve
         * @return {promise}        promise resolves with a array of catalogInfos
         */
         function getCatalogInfoArray(otherUserGameInfoArray) {
            return GamesDataFactory.getCatalogInfo(_.map(otherUserGameInfoArray, 'productId'))
                        .then(_.partial(transformData, otherUserGameInfoArray));
        }

        /**
         * merges catalogInfo list with gameInfo list
         * @param  {object[]} list of catalogInfos
         * @return {object[]} a list of offerIds with a .catalogInfo property marged into each object
         */
        function transformData(otherUserGameInfoArray, catalogInfos) {
            if (catalogInfos) {
                otherUserGameInfoArray.forEach(function(ownedGameInfo) {
                    if (ownedGameInfo.productId && catalogInfos.hasOwnProperty(ownedGameInfo.productId)) {
                        ownedGameInfo.masterTitleId = catalogInfos[ownedGameInfo.productId].masterTitleId;
                    }
                });
            }

            return otherUserGameInfoArray;
        }

        function handleGamesOwnedError(error) {
            if (error.message.indexOf('403') >= 0) {
                RosterDataFactory.setProfilePrivacy($scope.nucleusId, true);
            }
            $scope.loading = false;
            $scope.$digest();
            ComponentsLogFactory.error('origin-profile-page-games-otheruser handleGamesOwnedError:', error);
        }

        function onUpdate() {
            if (AuthFactory.isAppLoggedIn()) {
                var promiseArray = [Origin.atom.atomGamesOwnedForUser($scope.nucleusId).
                                        then(getCatalogInfoArray).
                                        then(transformData), 
                                    CommonGamesFactory.getCommonGames([$scope.nucleusId])];
                Promise.all(promiseArray).
                    then(_.spread(processGamesOwned)).
                    catch(handleGamesOwnedError);
            }
        }

        ObserverFactory.create(RosterDataFactory.getBlockListWatch())
            .attachToScope($scope, onUpdate);

        $scope.$on('$destroy', function () {
            //disconnect listening for events
            if(entitlementUpdateHandle) {
                entitlementUpdateHandle.detach();
            }
        });

        //setup a listener for any entitlement updates
        entitlementUpdateHandle = GamesDataFactory.events.on('games:baseGameEntitlementsUpdate', onUpdate, this);

        //if we already have data, lets use the entitlement info thats already there
        if (GamesDataFactory.initialRefreshCompleted()) {
            onUpdate();
        }

        $scope.showMoreGamesCommon = function () {
            var commonGamesTotal = $scope.commonGames.length;
            $scope.commonGamesLimit = commonGamesTotal;
            $scope.gamesYouBothOwnLoc = UtilFactory.getLocalizedStr($scope.gamesYouBothOwnStr, CONTEXT_NAMESPACE, 'gamesyoubothownstr')
                .replace('%number%', $scope.commonGamesLimit)
                .replace('%total%', commonGamesTotal);
        };

        $scope.showLessGamesCommon = function () {
            var commonGamesTotal = $scope.commonGames.length;
            $scope.commonGamesLimit = $scope.commonGamesInitialLimit;
            if (commonGamesTotal < $scope.commonGamesLimit) {
                $scope.commonGamesLimit = commonGamesTotal;
            }
            $scope.gamesYouBothOwnLoc = UtilFactory.getLocalizedStr($scope.gamesYouBothOwnStr, CONTEXT_NAMESPACE, 'gamesyoubothownstr')
                .replace('%number%', $scope.commonGamesLimit)
                .replace('%total%', commonGamesTotal);
        };

        $scope.showMoreGamesUncommon = function () {
            var uncommonGamesTotal = $scope.uncommonGames.length;
            $scope.uncommonGamesLimit = uncommonGamesTotal;

            $scope.gamesYouDontOwnLoc = UtilFactory.getLocalizedStr($scope.gamesYouDontOwnStr, CONTEXT_NAMESPACE, 'gamesyoudontownstr')
                .replace('%number%', $scope.uncommonGamesLimit)
                .replace('%total%', uncommonGamesTotal);

            $scope.otherGamesLoc = UtilFactory.getLocalizedStr($scope.otherGamesStr, CONTEXT_NAMESPACE, 'othergamesstr')
                .replace('%number%', $scope.uncommonGamesLimit)
                .replace('%total%', uncommonGamesTotal);
        };

        $scope.showLessGamesUncommon = function () {
            var uncommonGamesTotal = $scope.uncommonGames.length;
            $scope.uncommonGamesLimit = $scope.uncommonGamesInitialLimit;

            if (uncommonGamesTotal < $scope.uncommonGamesLimit) {
                $scope.uncommonGamesLimit = uncommonGamesTotal;
            }

            $scope.gamesYouDontOwnLoc = UtilFactory.getLocalizedStr($scope.gamesYouDontOwnStr, CONTEXT_NAMESPACE, 'gamesyoudontownstr')
                .replace('%number%', $scope.uncommonGamesLimit)
                .replace('%total%', uncommonGamesTotal);

            $scope.otherGamesLoc = UtilFactory.getLocalizedStr($scope.otherGamesStr, CONTEXT_NAMESPACE, 'othergamesstr')
                .replace('%number%', $scope.uncommonGamesLimit)
                .replace('%total%', uncommonGamesTotal);
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originProfilePageGamesOtheruser
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} nucleusid nucleusId of the user
     * @param {LocalizedString} gamesyoubothownstr "Games You Both Own - %number% of %total%"
     * @param {LocalizedString} gamesyoudontownstr "Games You Don't Own - %number% of %total%"
     * @param {LocalizedString} othergamesstr "Other Games - %number% of %total%"
     * @param {LocalizedString} showmoregamesstr "Show More Games"
     * @param {LocalizedString} showlessgamesstr "Show Less Games"
     * @param {LocalizedString} privatestr "You do not have permission to view this profile."
     * @param {LocalizedString} nogamesstr "This person doesn't have any games yet."
     * @description
     *
     * Profile Page - Games - Self
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-profile-page-games-otheruser nucleusid="123456789"
     *            gamesyoubothownstr="Games You Both Own - %number% of %total%"
     *            gamesyoudontownstr="Games You Don't Own - %number% of %total%"
     *            othergamesstr="Other Games - %number% of %total%"
     *            showmoregamesstr="Show More Games"
     *            showlessgamesstr="Show Less Games"
     *            privatestr="You do not have permission to view this profile."
     *            nogamesstr="This person doesn't have any games yet."
     *         ></origin-profile-page-games-self>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginProfilePageGamesOtheruserCtrl', OriginProfilePageGamesOtheruserCtrl)
        .directive('originProfilePageGamesOtheruser', function (ComponentsConfigFactory) {

            return {
                restrict: 'E',
                controller: 'OriginProfilePageGamesOtheruserCtrl',
                scope: {
                    nucleusId: '@nucleusid',
                    gamesYouBothOwnStr: '@gamesyoubothownstr',
                    gamesYouDontOwnStr: '@gamesyoudontownstr',
                    otherGamesStr: '@othergamesstr',
                    showMoreGamesStr: '@showmoregamesstr',
                    showLessGamesStr: '@showlessgamesstr',
                    privateStr: '@privatestr',
                    noGamesStr: '@nogamesstr'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('profile/page/games/views/otheruser.html')
            };

        });
}());

