(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginProfilePageGamesOtheruserCtrl($scope, GamesDataFactory, ComponentsLogFactory, ObjectHelperFactory, UtilFactory, RosterDataFactory, ComponentsConfigFactory) {

        var CONTEXT_NAMESPACE = 'origin-profile-page-games-otheruser';

        $scope.showMoreGamesLoc = UtilFactory.getLocalizedStr($scope.showMoreGamesStr, CONTEXT_NAMESPACE, 'showmoregamesstr');
        $scope.showLessGamesLoc = UtilFactory.getLocalizedStr($scope.showLessGamesStr, CONTEXT_NAMESPACE, 'showlessgamesstr');
        $scope.privateLoc = UtilFactory.getLocalizedStr($scope.privateStr, CONTEXT_NAMESPACE, 'privatestr');
        $scope.noGamesLoc = UtilFactory.getLocalizedStr($scope.noGamesStr, CONTEXT_NAMESPACE, 'nogamesstr');

        var getProperty = ObjectHelperFactory.getProperty;

        $scope.isPrivate = false;
        $scope.loading = false;
        $scope.commonGames = [];
        $scope.uncommonGames = [];
        $scope.commonGamesInitialLimit = 5;
        $scope.uncommonGamesInitialLimit = 10;
        $scope.commonGamesLimit = $scope.commonGamesInitialLimit;
        $scope.uncommonGamesLimit = $scope.uncommonGamesInitialLimit;

        function processGamesOwned(response) {

            var myOfferIds = getOwnedOfferIds();

            response.forEach(function (element, index) {
                var imageUrl;
                index = index;
                if (!!element.packArtLarge) {
                    imageUrl = 'https://eaassets-a.akamaihd.net/origin-com-store-final-assets-prod' + element.packArtLarge;
                }
                else {
                    // No box art, use placeholder image
                    imageUrl = ComponentsConfigFactory.getImagePath('profile\\game-placeholder.jpg');
                }
                if (myOfferIds.indexOf(element.productId) >= 0) {
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

            $scope.loading = false;
            $scope.isPrivate = false;
            RosterDataFactory.setProfilePrivacy(false);
            $scope.$digest();
        }

        function handleGamesOwnedError(error) {
            $scope.loading = false;
            if (error.message.indexOf('403') >= 0) {
                $scope.isPrivate = true;
                RosterDataFactory.setProfilePrivacy(true);
            }
                $scope.$digest();
            ComponentsLogFactory.error('origin-profile-page-games-otheruser atomGamesOwnedForUser:', error.message);
        }

        function getOwnedOfferIds() {
            return GamesDataFactory.baseGameEntitlements().map(getProperty('offerId'));
        }

        function onUpdate() {

            $scope.commonGames = [];
            $scope.uncommonGames = [];

            Origin.atom.atomGamesOwnedForUser($scope.nucleusId)
                .then(processGamesOwned)
                .catch(handleGamesOwnedError);
        }

        this.update = function() {
            $scope.loading = true;
            onUpdate();
        };

        function handleBlocklistChange() {
            onUpdate();
        }

        $scope.$on('$destroy', function () {
            //disconnect listening for events
            GamesDataFactory.events.off('games:baseGameEntitlementsUpdate', onUpdate);
            RosterDataFactory.events.off('xmppBlockListChanged', handleBlocklistChange);
        });

        //setup a listener for any entitlement updates
        GamesDataFactory.events.on('games:baseGameEntitlementsUpdate', onUpdate, this);

        //if we already have data, lets use the entitlement info thats already there
        if (GamesDataFactory.initialRefreshCompleted()) {
            onUpdate();
        }

        RosterDataFactory.events.on('xmppBlockListChanged', handleBlocklistChange);

        this.showMoreGamesCommon = function () {
            var commonGamesTotal = $scope.commonGames.length;
            $scope.commonGamesLimit = commonGamesTotal;
            $scope.gamesYouBothOwnLoc = UtilFactory.getLocalizedStr($scope.gamesYouBothOwnStr, CONTEXT_NAMESPACE, 'gamesyoubothownstr')
                .replace('%number%', $scope.commonGamesLimit)
                .replace('%total%', commonGamesTotal);
            $scope.$digest();
        };

        this.showLessGamesCommon = function () {
            var commonGamesTotal = $scope.commonGames.length;
            $scope.commonGamesLimit = $scope.commonGamesInitialLimit;
            if (commonGamesTotal < $scope.commonGamesLimit) {
                $scope.commonGamesLimit = commonGamesTotal;
            }
            $scope.gamesYouBothOwnLoc = UtilFactory.getLocalizedStr($scope.gamesYouBothOwnStr, CONTEXT_NAMESPACE, 'gamesyoubothownstr')
                .replace('%number%', $scope.commonGamesLimit)
                .replace('%total%', commonGamesTotal);
            $scope.$digest();
        };

        this.showMoreGamesUncommon = function () {
            var uncommonGamesTotal = $scope.uncommonGames.length;
            $scope.uncommonGamesLimit = uncommonGamesTotal;

            $scope.gamesYouDontOwnLoc = UtilFactory.getLocalizedStr($scope.gamesYouDontOwnStr, CONTEXT_NAMESPACE, 'gamesyoudontownstr')
                .replace('%number%', $scope.uncommonGamesLimit)
                .replace('%total%', uncommonGamesTotal);

            $scope.otherGamesLoc = UtilFactory.getLocalizedStr($scope.otherGamesStr, CONTEXT_NAMESPACE, 'othergamesstr')
                .replace('%number%', $scope.uncommonGamesLimit)
                .replace('%total%', uncommonGamesTotal);

            $scope.$digest();
        };

        this.showLessGamesUncommon = function () {
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

            $scope.$digest();
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originProfilePageGamesOtherUser
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
        .directive('originProfilePageGamesOtheruser', function (ComponentsConfigFactory, RosterDataFactory) {

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
                templateUrl: ComponentsConfigFactory.getTemplatePath('profile/page/games/views/otheruser.html'),
                link: function (scope, element, attrs, ctrl) {
                    scope = scope;
                    attrs = attrs;

                    scope.$on('activePageChange', function(event, data) {
                        if (data === 'games') {
                            // reload this page if this is a stranger profile. This is required in case this user has blocked me.
                            var rosterFriend = RosterDataFactory.getRosterFriend(scope.nucleusId);
                            var strangerProfile = (!rosterFriend || rosterFriend.subState !== 'BOTH');
                            if (strangerProfile) {
                                ctrl.update();
                            }
                        }
                    });

                    // Listen for clicks on the "show more / show less" links
                    $(element).on('click', '.show-more-games-common', function () {
                        ctrl.showMoreGamesCommon();
                    });
                    $(element).on('click', '.show-less-games-common', function () {
                        ctrl.showLessGamesCommon();
                    });
                    $(element).on('click', '.show-more-games-uncommon', function () {
                        ctrl.showMoreGamesUncommon();
                    });
                    $(element).on('click', '.show-less-games-uncommon', function () {
                        ctrl.showLessGamesUncommon();
                    });

                }
            };

        });
}());

