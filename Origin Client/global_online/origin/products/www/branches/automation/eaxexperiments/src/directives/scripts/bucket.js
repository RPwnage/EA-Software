/**
 * @file bucket.js
 */

/*jshint unused: false */
(function() {
    'use strict';

    function EaxExperimentBucketCtrl($scope, ExperimentFactory) {

        function setInExperimentResult(forceUpdate, result) {
            if (forceUpdate || ($scope.inExperiment !== result)) {
                $scope.inExperiment = result;
                $scope.$digest();
            }
        }

        $scope.updateInExperiment = function(forceUpdate) {
            ExperimentFactory.inExperiment($scope.expname, $scope.expvariant)
                .then(_.partial(setInExperimentResult, forceUpdate));
        };


        function onDestroy() {
            ExperimentFactory.events.off('experiment:setUser', $scope.updateInExperiment);
        }

        $scope.$on('$destroy', onDestroy);

        //listen for setUser which is called when auth changes
        ExperimentFactory.events.on('experiment:setUser', $scope.updateInExperiment);
    }

    function eaxExperimentBucket(ExperimentFactory) {

        function eaxExperimentBucketLink(scope) {
            scope.updateInExperiment(true);
        }

        return {
            scope : {
                expname: '@',
                expvariant: '@'
            },
            restrict: 'E',
            transclude: true,
            controller: EaxExperimentBucketCtrl,
            link: eaxExperimentBucketLink,
            template: '<div ng-if="inExperiment"><div ng-transclude></div></div>'
        };
    }

    /**
     * @ngdoc directive
     * @name eax-experiments.directives:eaxExperimentBucket
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} expname experiment name
     * @param {string} expvariant experiment variant name
     * @description
     *
     * bucket used to enable/disable an element based on experiment specified by expname and expvariant
     *
     *
     */
    angular.module('eax-experiments')
        .directive('eaxExperimentBucket', eaxExperimentBucket);
}());