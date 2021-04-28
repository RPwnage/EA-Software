
/** 
 * @file /store/sitestripe/scripts/Alert.js
 */ 
(function(){
    'use strict';
    /**
    * The controller
    */
    function OriginStoreSitestripeAlertCtrl($scope) {
    	// enable/disable mod
    	$scope.hideMod = localStorage.omotd === 'z' ? true : false;
    	
    	$scope.edgecolor = '#c93507';
    	$scope.alert = true;
    }
    /**
    * The directive
    */
    function originStoreSitestripeAlert(ComponentsConfigFactory) {
        /**
        * The directive link
        */
        function originStoreSitestripeAlertLink(scope, elem) {
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
            controller: 'OriginStoreSitestripeAlertCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('/store/sitestripe/views/messageoftheday.html'),
            link: originStoreSitestripeAlertLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreSitestripeAlert
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
     *     <origin-store-sitestripe-alert
     *     		title="some title">
     *     </origin-store-sitestripe-alert>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreSitestripeAlertCtrl', OriginStoreSitestripeAlertCtrl)
        .directive('originStoreSitestripeAlert', originStoreSitestripeAlert);
}());