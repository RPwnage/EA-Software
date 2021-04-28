
/** 
 * @file store/sitestripe/scripts/wrapper.js
 */ 
(function(){
    'use strict';
    /**
    * The controller
    */
    function OriginStoreSitestripeWrapperCtrl($scope, $http, APPCONFIG, LogFactory, $compile, $timeout) {

    	// check the server for me MotD every 5 min
    	this.checkForNewMod = function(elem) {
    		
    		$http.get(APPCONFIG.app.storeModUrl)
	            .success(function(response) {
	            	
	            	if(response.length > 0) {
		            	var newModDate = angular.element(response).attr('last-modified');
		            	
		            	if(Date.parse(newModDate) > Date.parse($scope.lastModified)) {
							elem.slideUp(function(){
								// show the new mod
								localStorage.removeItem('omotd');
								var newMod = angular.element(response).children();
								angular.element(elem).children().remove();
								angular.element(elem).append($compile(newMod)($scope));
								
								$timeout(function(){
									$(elem).slideDown();
								}, 250);
								
							});
							
							$scope.lastModified = newModDate;
		            	}
	            	}
    				
	            }).error(function(data, status) {
	                LogFactory.log(status, ' - retrieveStoreModWrapperData');
	            });
    		
    	};
    	
    }
    /**
    * The directive
    */
    function originStoreSitestripeWrapper(ComponentsConfigFactory, $interval) {
        /**
        * The directive link
        */
        function originStoreSitestripeWrapperLink(scope, elem, attrs, ctrl) {
            // Add Scope functions and call the controller from here
        	
        	$interval(function(){
            	ctrl.checkForNewMod(elem);
            }, 300000);
        	
        }
        return {
            restrict: 'E',
            transclude: true,
            replace: true,
            scope: {
            	lastModified: '@'
            },
            controller: 'OriginStoreSitestripeWrapperCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/sitestripe/views/wrapper.html'),
            link: originStoreSitestripeWrapperLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreSitestripeWrapper
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {string} last-modified The date and time the mod page was last modified.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     		<origin-store-sitestripe-wrapper
     *     			last-modified="June 5, 2015 06:00:00 AM PDT">		
     *     		</origin-store-sitestripe-wrapper>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreSitestripeWrapperCtrl', OriginStoreSitestripeWrapperCtrl)
        .directive('originStoreSitestripeWrapper', originStoreSitestripeWrapper);
}()); 
