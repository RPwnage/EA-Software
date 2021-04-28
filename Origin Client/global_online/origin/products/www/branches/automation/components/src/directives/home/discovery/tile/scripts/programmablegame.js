/**
 * @file home/discovery/tile/programmablegame.js
 */
(function() {
    'use strict';

    /* jshint ignore:start */
    var FriendsListEnumeration = {
        "friendsPlayedOrPlaying": "friendsPlayedOrPlaying",
        "friendsOwned": "friendsOwned"
    };

    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    function originHomeDiscoveryTileProgrammableGame(ComponentsConfigFactory) {

        function originHomeDiscoverTileProgrammableGameLink(scope, elem, attr, ctrl, $transclude) {
            $transclude(function(clone) {
                //if there are no elements passed in for transclude it means we have no buttons
                //so don't show the hover
                scope.showHover = clone.length;
            });
        }

        return {
            restrict: 'E',
            transclude: true,
            replace: true,
            scope: {
                gamename: '@gamename',
                image: '@discoverTileImage',
                descriptionRaw: '@description',
                offerId: '@offerid',
                requestedFriendType: '@friendslisttype',
                friendsText: '@friendstext',
                friendsPopoutTitle: '@friendspopouttitle',
                showPrice: '@showprice',
                ocdPath: '@',
                ctid: '@',
                recoid:'@',
                recoindex:'@',
                recotag:'@',
                recotype: '@'

            },
            controller: 'OriginHomeDiscoveryTileProgrammableCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/discovery/tile/views/programmablegame.html'),
            link: originHomeDiscoverTileProgrammableGameLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileProgrammableGame
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} ctid the Content Tracking ID used for telemetry
     * @param {LocalizedString} gamename the name of the game, if not passed it will use the name from catalog
     * @param {ImageUrl} image the image to use
     * @param {LocalizedText} description the description text
     * @param {string} offerid the offerid associated with the programmable tile
     * @param {OcdPath} ocd-path the path associated with the programmable tile
     * @param {FriendsListEnumeration} requestedfriendtype the requested friend type if there is one
     * @param {LocalizedText} friendstext the friends text associated with the avatar list
     * @param {LocalizedText} friendspopouttitle the title shown on the friends popout title
     * @param {BooleanEnumeration} showprice do we want to show a price if there is a valid offer id associated with the programmable tile
     * @param {string} recotag a tag returned by the recommendation API that we need to send for tracking
     * @param {string} recoindex the index of where the tile sits in the bucket
     * @param {string} recoid the unique id of the tile (ctid for news/ project id for games)
     * @param {string} recotype the type of the recommendation (news/games)

     * @description
     *
     * programmable discovery tile
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-discovery-tile-programmable-game image="/content/dam/originx/web/app/home/discovery/programmed/tile_add_friends_long.jpg" description="<strong>SIMS 4 GAME TIME</strong><br>Are you ready to rule? The Sims 4 is now available with Origin Game Time, giving players 48 hours of free, unrestricted gameplay.">
     *             <origin-cta-directacquisition actionid="direct-acquistion" href="pathtogamelibrary" description="Get It Now" type="primary"></origin-cta-directacquisition>
     *             <origin-cta-loadurlexternal actionid="url-external" href="https://www.origin.com/en-us/news/the-sims-4-game-time" description="Read More" type="secondary"></origin-cta-loadurlexternal>
     *         </origin-home-discovery-tile-programmable>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originHomeDiscoveryTileProgrammableGame', originHomeDiscoveryTileProgrammableGame);
}());