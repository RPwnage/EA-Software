/**
 * @file home/discovery/tile/config/programmable.js
 */
(function() {
    'use strict';

    /* jshint ignore:start */
    //These values are used by discovery story factory
    var PlacementTypeEnumeration = {
        "priority": "priority",
        "position": "position"
    };

    var FriendsListEnumeration = {
        "friendsPlayedOrPlaying": "friendsPlayedOrPlaying",
        "friendsOwned": "friendsOwned"
    };

    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    function OriginHomeDiscoveryTileConfigProgrammableGameCtrl($scope, $attrs, $controller) {

        var tileConfig = {
                directiveName: 'origin-home-discovery-tile-programmable-game',
                feedData: [{
                    gamename: $attrs.gamename,
                    'discover-tile-image': $attrs.image, // Tealium uses this attribute
                    description: $attrs.description, // Tealium uses this attribute
                    friendslisttype: $attrs.friendslisttype,
                    friendstext: $attrs.friendstext,
                    friendspopouttitle: $attrs.friendspopouttitle,
                    showprice: $attrs.showprice,
                    ctid: $attrs.ctid, // Tealium uses this attribute
                    'ocd-path': $attrs.ocdPath // Tealium uses this attribute
                }],
                limit: 1,
                diminish: 0
        };

        if(!angular.isDefined($attrs.placementtype) || !angular.isDefined($attrs.placementvalue)) {
            tileConfig.position = 1;
        } else {
            tileConfig[$attrs.placementtype] = parseInt($attrs.placementvalue);
        }

        $scope.childDirectives = [];
        tileConfig.childDirectives = $scope.childDirectives;

        $controller('OriginHomeDiscoveryTileConfigCtrl', {
            $scope: $scope,
            tileConfig: tileConfig
        });
    }

    function originHomeDiscoveryTileConfigProgrammableGame() {
        return {
            restrict: 'E',
            controller: 'OriginHomeDiscoveryTileConfigProgrammableGameCtrl',
            scope: true
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileConfigProgrammableGame
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} ctid the Content Tracking ID used for telemetry
     * @param {LocalizedString} gamename the name of the game, if not passed it will use the name from catalog
     * @param {ImageUrl} image the image
     * @param {LocalizedString} description the description text
     * @param {FriendsListEnumeration} friendslisttype the friends list type you want to show
     * @param {LocalizedText} friendstext the friends text associated with the avatar list
     * @param {LocalizedText} friendspopouttitle the title shown on the friends popout title
     * @param {BooleanEnumeration} showprice do we want to show a price if there is a valid offer id associated with the programmable tile
     * @param {PlacementTypeEnumeration} placementtype the placement type
     * @param {Number} placementvalue the placement value
     * @param {string} offerid the offer id used for pricing/friends list
     * @param {OcdPath} ocd-path ocd path used for pricing/friends list
     * @description
     *
     * tile config for programmable game discovery tile
     *
     */
    //TODO: make this directive standard
    angular.module('origin-components')
        .controller('OriginHomeDiscoveryTileConfigProgrammableGameCtrl', OriginHomeDiscoveryTileConfigProgrammableGameCtrl)
        .directive('originHomeDiscoveryTileConfigProgrammableGame', originHomeDiscoveryTileConfigProgrammableGame);

}());