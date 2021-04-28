
/** 
 * @file /store/sitestripe/scripts/Info.js
 */ 
(function(){
    'use strict';
    /**
    * The controller
    */
    function OriginStoreSitestripeInfoCtrl($scope) {
    	// enable/disable mod
    	$scope.hideMod = localStorage.omotd === 'z' ? true : false;
    	
    	$scope.edgecolor = '#269dcf';
    	$scope.info = true;
    }
    /**
    * The directive
    */
    function originStoreSitestripeInfo(ComponentsConfigFactory) {
        /**
        * The directive link
        */
        function originStoreSitestripeInfoLink(scope, elem) {
            // Add Scope functions and call the controller from here
        	
        	elem.find('.origin-store-sitestripe-messageoftheday-close').on('click', function(){
				elem.css('height', 0);
				
				localStorage.omotd = 'z';
			});
        }
        return {
            restrict: 'E',
            replace: true,
            scope: {
            	title: '@'
            },
            controller: 'OriginStoreSitestripeInfoCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('/store/sitestripe/views/messageoftheday.html'),
            link: originStoreSitestripeInfoLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreSitestripeInfo
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} title The title for this site stripe.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-sitestripe-info
     *     		title="some title">
     *     </origin-store-sitestripe-info>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreSitestripeInfoCtrl', OriginStoreSitestripeInfoCtrl)
        .directive('originStoreSitestripeInfo', originStoreSitestripeInfo);
}());