/**
 * @file home/discovery/tile/config/programmableaction.js
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

    function OriginHomeDiscoveryTileConfigProgrammableActionCtrl($scope, $attrs, $controller) {

        var tileConfig = {
                directiveName: 'origin-home-discovery-tile-programmable-action',
                feedData: [{
                    image: $attrs.image,
                    descriptiontitle: $attrs.descriptiontitle,
                    description: $attrs.description,
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

    function originHomeDiscoveryTileConfigProgrammableAction() {
        return {
            restrict: 'E',
            controller: 'OriginHomeDiscoveryTileConfigProgrammableActionCtrl',
            scope: true
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileConfigProgrammableAction
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} ctid the Content Tracking ID used for telemetry
     * @param {ImageUrl} image the image
     * @param {LocalizedString} descriptiontitle the description title     
     * @param {LocalizedString} description the description text
     * @param {PlacementTypeEnumeration} placementtype the placement type
     * @param {Number} placementvalue the placement value
     * @description
     *
     * tile config for programmable game discovery tile
     *
     */
    //TODO: make this directive standard
    angular.module('origin-components')
        .controller('OriginHomeDiscoveryTileConfigProgrammableActionCtrl', OriginHomeDiscoveryTileConfigProgrammableActionCtrl)
        .directive('originHomeDiscoveryTileConfigProgrammableAction', originHomeDiscoveryTileConfigProgrammableAction);

}());