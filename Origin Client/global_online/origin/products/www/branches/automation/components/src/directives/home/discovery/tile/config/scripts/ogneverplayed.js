 /**
  * @file home/discovery/tile/config/ogneverplayed.js
  */
 (function() {
     'use strict';

     function OriginHomeDiscoveryTileConfigOgneverplayedCtrl($scope, $attrs, $controller) {
         $controller('OriginHomeDiscoveryTileConfigCtrl', {
             tileConfig: {
                 directiveName: 'origin-home-discovery-tile-ogneverplayed',
                 feedFunctionName: 'getNeverPlayed',
                 priority: parseInt($attrs.priority),
                 diminish: parseInt($attrs.diminish),
                 limit: parseInt($attrs.limit),
                 neverplayedsince: parseInt($attrs.neverplayedsince)
             },
             $scope: $scope
         });
     }

     function originHomeDiscoveryTileConfigOgneverplayed() {
         return {
             restrict: 'E',
             controller: 'OriginHomeDiscoveryTileConfigOgneverplayedCtrl',
             scope: true
         };
     }


     /**
      * @ngdoc directive
      * @name origin-components.directives:originHomeDiscoveryTileConfigOgneverplayed
      * @restrict E
      * @element ANY
      * @scope
      * @param {number} priority The priority assigned via CQ5 to this type
      * @param {number} diminish The value to diminish the priority by after this type is used
      * @param {number} limit The maximum number of times this tile type can appear
      * @param {number} neverplayedsince the min number of days a user has not played this game
      * @description
      *
      * tile config for never played discovery tile
      *
      */
     angular.module('origin-components')
         .controller('OriginHomeDiscoveryTileConfigOgneverplayedCtrl', OriginHomeDiscoveryTileConfigOgneverplayedCtrl)
         .directive('originHomeDiscoveryTileConfigOgneverplayed', originHomeDiscoveryTileConfigOgneverplayed);
 }());