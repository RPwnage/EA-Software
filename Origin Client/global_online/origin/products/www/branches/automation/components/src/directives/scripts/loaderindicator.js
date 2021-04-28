
/**
 * @file /scripts/lazyloader.js
 */
(function(){
    'use strict';
	
    function originLoaderindicatorCtrl($scope, ComponentsConfigFactory) {
        $scope.spinnerImage = ComponentsConfigFactory.getImagePath('loader.png');
    }

    function originLoaderindicator(ComponentsConfigFactory) {

        return {
            restrict: 'E',
            controller: 'originLoaderindicatorCtrl',
			scope: {},
            templateUrl: ComponentsConfigFactory.getTemplatePath('/views/loaderindicator.html')        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originLoaderindicator
     * @restrict E
     * @element ANY
     * @description
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('originLoaderindicatorCtrl', originLoaderindicatorCtrl)
        .directive('originLoaderindicator', originLoaderindicator);
}());
