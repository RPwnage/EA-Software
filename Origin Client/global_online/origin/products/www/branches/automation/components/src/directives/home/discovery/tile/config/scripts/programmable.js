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

    function OriginHomeDiscoveryTileConfigProgrammableCtrl($scope, $attrs, $controller) {
        var tileConfig = {
            directiveName: 'origin-home-discovery-tile-programmable',
            feedData: [{
                image: $attrs.image, // Tealium uses this attribute
                subtitle: $attrs.subtitle, // Tealium uses this attribute
                description: $attrs.description, // Tealium uses this attribute
                authorname: $attrs.authorname,
                creationdate: $attrs.creationdate,
                authordescription: $attrs.authordescription,
                authorhref: $attrs.authorhref,
                timetoread: $attrs.timetoread,
                friendslisttype: $attrs.friendslisttype,
                friendstext: $attrs.friendstext,
                friendspopouttitle: $attrs.friendspopouttitle,
                offerid: $attrs.offerid, // Tealium uses this attribute
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

    function originHomeDiscoveryTileConfigProgrammable() {
        return {
            restrict: 'E',
            controller: 'OriginHomeDiscoveryTileConfigProgrammableCtrl',
            scope: true
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileConfigProgrammable
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} ctid the Content Tracking ID used for telemetry
     * @param {ImageUrl} image the image
     * @param {LocalizedString} subtitle the subtitle text
     * @param {LocalizedString} description the description text
     * @param {LocalizedString} authorname an optional author name eg. JohnDoe253
     * @param {LocalizedString} authordescription an optional author description eg. Youtube User
     * @param {LocalizedString} timetoread an optional time to read/watch description eg. 4 min read
     * @param {Url} authorhref an optional author external link
     * @param {FriendsListEnumeration} friendslisttype the friends list type you want to show
     * @param {LocalizedText} friendstext the friends text associated with the avatar list
     * @param {LocalizedText} friendspopouttitle the title shown on the friends popout title
     * @param {BooleanEnumeration} showprice do we want to show a price if there is a valid offer id associated with the programmable tile
     * @param {PlacementTypeEnumeration} placementtype the placement type
     * @param {Number} placementvalue the placement value
     * @param {string} offerid the offer id used for pricing/friends list
     * @param {OcdPath} ocd-path ocd path used for pricing/friends list
     * @param {DateTime} creationdate date when the tile is created
     * @description
     *
     * tile config for programmable tile discovery tile
     *
     */
    //TODO: make this directive standard
    angular.module('origin-components')
        .controller('OriginHomeDiscoveryTileConfigProgrammableCtrl', OriginHomeDiscoveryTileConfigProgrammableCtrl)
        .directive('originHomeDiscoveryTileConfigProgrammable', originHomeDiscoveryTileConfigProgrammable);

}());