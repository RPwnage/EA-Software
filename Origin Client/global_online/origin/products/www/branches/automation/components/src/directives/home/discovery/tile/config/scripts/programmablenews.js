/**
 * @file home/discovery/tile/config/programmablenews.js
 */
(function() {
    'use strict';

    /* jshint ignore:start */
    //These values are used by discovery story factory
    var PlacementTypeEnumeration = {
        "priority": "priority",
        "position": "position"
    };

    /* jshint ignore:end */

    function OriginHomeDiscoveryTileConfigProgrammableNewsCtrl($scope, $attrs, $controller) {

        var tileConfig = {
                directiveName: 'origin-home-discovery-tile-programmable-news',
                feedData: [{
                    image: $attrs.image,
                    descriptiontitle: $attrs.descriptiontitle,
                    description: $attrs.description,
                    authorname: $attrs.authorname,
                    creationdate: $attrs.creationdate,
                    authordescription: $attrs.authordescription,
                    authorhref: $attrs.authorhref,
                    timetoread: $attrs.timetoread,
                    ctid: $attrs.ctid
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

    function originHomeDiscoveryTileConfigProgrammableNews() {
        return {
            restrict: 'E',
            controller: 'OriginHomeDiscoveryTileConfigProgrammableNewsCtrl',
            scope: true
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileConfigProgrammableNews
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} ctid the Content Tracking ID used for telemetry
     * @param {ImageUrl} image the image to use
     * @param {LocalizedText} descriptiontitle the news header
     * @param {LocalizedText} description the description text
     * @param {LocalizedString} authorname an optional author name eg. JohnDoe253
     * @param {LocalizedString} authordescription an optional author description eg. Youtube User
     * @param {LocalizedString} timetoread an optional time to read/watch description eg. 4 min read
     * @param {Url} authorhref an optional author external link
     * @param {PlacementTypeEnumeration} placementtype the placement type
     * @param {Number} placementvalue the placement value
     * @param {DateTime} creationdate date when the tile is created
     * @description
     *
     * tile config for programmable game discovery tile
     *
     */
    //TODO: make this directive standard
    angular.module('origin-components')
        .controller('OriginHomeDiscoveryTileConfigProgrammableNewsCtrl', OriginHomeDiscoveryTileConfigProgrammableNewsCtrl)
        .directive('originHomeDiscoveryTileConfigProgrammableNews', originHomeDiscoveryTileConfigProgrammableNews);

}());