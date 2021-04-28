 /**
  * @file home/discovery/tile/config/newsautorec.js
  */
 (function() {
     'use strict';

     function OriginHomeDiscoveryTileConfigNewsautorecCtrl($scope, $attrs, PinRecommendationNewsFactory) {
         //set a flag so that every tile under this component will be considered for recommendation
         $scope.pinRecommendation = true;

         //let the bucket know of the filter function to use
         $scope.$emit('bucketFilter', {
             filterFunction: _.partialRight(PinRecommendationNewsFactory.filterRecommendedNews, parseInt($attrs.numrecs))
         });
     }

     function originHomeDiscoveryTileConfigNewsautorec() {
         return {
             restrict: 'E',
             controller: 'OriginHomeDiscoveryTileConfigNewsautorecCtrl',
             scope: false
         };
     }


     /**
      * @ngdoc directive
      * @name origin-components.directives:originHomeDiscoveryTileConfigNewsautorec
      * @restrict E
      * @element ANY
      * @scope
      * @param {number} numrecs The maximum number of recommendations we want returned
      * @description
      *
      * wrapper for tiles we want to run through pin reco
      *
      */
     angular.module('origin-components')
         .controller('OriginHomeDiscoveryTileConfigNewsautorecCtrl', OriginHomeDiscoveryTileConfigNewsautorecCtrl)
         .directive('originHomeDiscoveryTileConfigNewsautorec', originHomeDiscoveryTileConfigNewsautorec);
 }());