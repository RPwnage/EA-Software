/**
 * @file Acheivement/posterTile.js
 */
(function() {
    'use strict';

    /* jshint ignore:start */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    var CONTEXT_NAMESPACE = 'origin-achievement-tile';

    /**
     * Origin Achievement Tile Ctrl
     */
    function OriginAchievementTileCtrl($scope, AuthFactory, AchievementFactory, ComponentsLogFactory, UtilFactory, AppCommFactory, GamesDataFactory, PdpUrlFactory, OcdHelperFactory, ComponentsConfigFactory, NavigationFactory) {

        var entitlement = GamesDataFactory.getBaseGameEntitlementByAchievementSetId($scope.achievementSetId),
            localize = UtilFactory.getLocalizedStrGenerator(CONTEXT_NAMESPACE),
            isOwned = !!entitlement,
            ocdPath;

        /**
         * Set default banner for gametile image
         * @type {String}
         */
        $scope.image = ComponentsConfigFactory.getImagePath('achievements-default-banner.png');

        /**
         * Is touch disabled on this device
         * @type {Boolean}
         */
        $scope.isTouchDisable = !UtilFactory.isTouchEnabled();

        /* Get Translated Strings */
        $scope.viewDetailsLoc = localize($scope.viewDetailsStr, 'viewdetails');
        $scope.achievementsLoc = localize($scope.achievementsStr, 'achievements');
        $scope.pointsLoc = localize($scope.pointsStr, 'points');

        /**
         * Go to Achievement Details Page with the selected achievementSetId
         * @return {void}
         * @method {navigateToDetails}
         */
        function navigateToDetails() {
            var isSelf = Number($scope.personaId) === Number(Origin.user.personaId());

            if (isSelf) {
                NavigationFactory.goToState('app.profile.topic.details', {
                    topic: 'achievements',
                    detailsId: $scope.achievementSetId
                });
            } else {
                NavigationFactory.goUserAchievementDetails('app.profile.user.topic.details', {
                    id: $scope.nucleusId,
                    topic: 'achievements',
                    detailsId: $scope.achievementSetId
                });
            }
        }

        /**
         * Go to Origin Game Details Page with the selected offerId
         * @return {void}
         * @method {navigateToOGD}
         */
        function navigateToOGD() {
            AppCommFactory.events.fire('uiRouter:go', 'app.game_gamelibrary.ogd', {
                offerid: entitlement.offerId
            });
        }

        /**
         * Go to Store Product Details Page with the selected ocd path elements
         * @return {void}
         * @method {navigateToPDP}
         */
        function navigateToPDP() {
            if(ocdPath) {
                var offerData = {
                    ocdPath: ocdPath
                };
                PdpUrlFactory.goToPdp(offerData);
            }
        }

        /**
         * Executes when CTA is clicked - or when tile is touched, if touch enabled.
         * @return {void}
         * @method {onCtaClicked}
         */
        $scope.onCtaClicked = function() {
            if ($scope.isDetailsPage) {
                if (isOwned) {
                    navigateToOGD();
                }
                else {
                    navigateToPDP();
                }
            } else { // on summary page
                navigateToDetails();
            }
        };

        /**
         * Executes when the Title is clicked.
         * @return {void}
         * @method {onTitleClicked}
         */
        $scope.onTitleClicked = function() {
            if (isOwned) {
                navigateToOGD();
            } else {
                navigateToPDP();
            }
        };

        /**
         * Set the achievement game image.
         * @method {setAchievementOcdData}
         */
        function setAchievementOcdData(data) {
            if(data !== undefined && _.has(data, 'discover-tile-image')) {
                $scope.image = _.get(data, 'discover-tile-image');
            }
            $scope.$digest();
        }

        /**
         * Set the ocd path and get the OCD data for game.
         * @method {getAchievementOcdData}
         */
        function getAchievementOcdData(path) {
             // SDK won't have a path so we need the check
            if(path){
                ocdPath = path; //set for navigateToPDP method
                GamesDataFactory.getOcdByPath(ocdPath)
                    .then(OcdHelperFactory.findDirectives('origin-game-defaults'))
                    .then(OcdHelperFactory.flattenDirectives('origin-game-defaults'))
                    .then(_.head)
                    .then(setAchievementOcdData)
                    .catch(function(error) {
                        ComponentsLogFactory.error('[Origin-Achievement-Tile-Directive getAchievementOcdData Method] error', error);
                    });
            }
        }

        /**
         * Get the OCD path for the game using master title id.
         * @method getOcdPath
         */
        function getOcdPath(){
            var masterTitleId = $scope.achievementSetId.split('_')[1];

            // We have to handle a special case for BF3.  Its achievement set ID is "BF_BF3_PC".  This is the only achievement set ID
            // that does not use the numeric ID's, for historical reasons.
            if (masterTitleId === "BF3") {
                masterTitleId = 50182;
            }

            GamesDataFactory.getPurchasablePathByMasterTitleId(masterTitleId, true)
                .then(getAchievementOcdData)
                .catch(function(error) {
                    ComponentsLogFactory.error('[Origin-Achievement-Tile-Directive getAchievementSet Method] error', error);
                });
        }

        /**
         * Set achievement game title and score
         * @method setAchievementData
         */
        function setAchievementData(game) {
            $scope.titleStr = game.baseGame.name;
            $scope.score = {
                grantedAchievements: game.grantedAchievements(),
                totalAchievements: game.totalAchievements(),
                grantedXp: game.grantedXp(),
                totalXp: game.totalXp()
            };
            $scope.$digest();
        }

        /**
         * Get Achievement Data from achievement factory and initiate a call to get 
         * ocd path for PDP and OCD.
         * @method getAchievementData
         */
        function getAchievementData() {

            getOcdPath(); // get ocd path for pdp route and game image.

            AchievementFactory.getAchievementSet($scope.personaId, $scope.achievementSetId)
                .then(setAchievementData)
                .catch(function(error) {
                    ComponentsLogFactory.error('[Origin-Achievement-Tile-Directive getAchievementSet Method] error', error);
                });
        }

        //Initiate call to get achievmentSet from factory
        getAchievementData();

        /**
         * Unhook from auth factory events when directive is destroyed.
         * @method onDestroy
         */
        function onDestroy() {
            AuthFactory.events.off('myauth:ready', getAchievementData);
            AchievementFactory.events.off('achievements:achievementGranted', getAchievementData);
        }

        /* Bind to auth events */
        AuthFactory.events.on('myauth:ready', getAchievementData);
        AchievementFactory.events.on('achievements:achievementGranted', getAchievementData);

        $scope.$on('$destroy', onDestroy);
    }

    function originAchievementTile(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                achievementSetId: '@achievementsetid',
                personaId: '@personaid',
                nucleusId: '@nucleusid',
                isDetailsPage: '=isdetailspage',
                image: '@',
                viewDetailsStr: '@viewdetails',
                achievementsStr: '@achievements',
                pointsStr: '@points',
                titleStr: '@'
            },
            controller: OriginAchievementTileCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('achievements/views/tile.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originAchievementTile
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} achievementsetid the achievementSetId of the game you want to interact with
     * @param {string} personaid the perosnaId of the user in case of friends/stranger profile
     * @param {string} nucleusid the nucleusId of the user in case of friends/stranger profile
     * @param {string} title-str game name
     * @param {BooleanEnumeration} isdetailspage is this tile on the achievement details page, or the achievement summary page?
     * @param {ImageUrl} image the url of the image
     * @param {LocalizedString} viewdetails View Details view details takes the user to details page
     * @param {LocalizedString} achievements Achievements text
     * @param {LocalizedString} points Points text
     * @description
     *
     * Acheivement Poster Tile
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-achievement-tile achievementsetid="50563_52657_50844" personaid="123456789" nucleusid="12345678" isdetailspage="true"></origin-achievement-tile>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginAchievementTileCtrl', OriginAchievementTileCtrl)
        .directive('originAchievementTile', originAchievementTile);
}());
