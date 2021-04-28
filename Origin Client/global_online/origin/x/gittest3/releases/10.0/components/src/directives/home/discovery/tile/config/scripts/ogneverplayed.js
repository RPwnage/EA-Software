 /**
  * @file home/discovery/tile/config/ogneverplayed.js
  */
 (function() {
     'use strict';

     function originHomeDiscoveryTileConfigOgneverplayed(DiscoveryStoryFactory) {

         function originHomeDiscoveryTileConfigOgneverplayedLink(scope, element, attrs, parentCtrl) {
             var attributes = {
                 priority: scope.priority,
                 diminish: scope.diminish,
                 limit: scope.limit,
                 neverplayedsince: scope.neverplayedsince
             };
             DiscoveryStoryFactory.updateAutoStoryTypeConfig(parentCtrl.bucketId(), 'HT_05', attributes);
         }

         return {
             restrict: 'E',
             require: '^originHomeDiscoveryBucket',
             scope: {
                 priority: '@',
                 diminish: '@',
                 limit: '@',
                 neverplayedsince: '@'
             },
             link: originHomeDiscoveryTileConfigOgneverplayedLink
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
         .directive('originHomeDiscoveryTileConfigOgneverplayed', originHomeDiscoveryTileConfigOgneverplayed);
 }());