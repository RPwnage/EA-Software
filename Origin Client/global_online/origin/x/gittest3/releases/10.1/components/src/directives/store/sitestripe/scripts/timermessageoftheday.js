
/** 
 * @file /store/sitestripe/scripts/Timermessageoftheday.js
 */ 
(function(){
    'use strict';
    /**
    * The controller
    */
    function OriginStoreSitestripeTimermessageofthedayCtrl($scope) {
    	// enable/disable mod
    	$scope.hideMod = localStorage.omotd === 'z' ? true : false;
    	
    	// enable the timer
    	$scope.timer = true;
    	
    	// Handle the timer expiry
    	$scope.$on('timerReached', function() {
    		if($scope.newtitle) {
    			$scope.title = $scope.newtitle;
    		}
    		
    		if($scope.newsubtitle) {
    			$scope.subtitle = $scope.newsubtitle;
    		}
    		
    		if($scope.hidetimer === 'true') {
    			$scope.timer = false;
    		}
        });
    	
    }
    /**
    * The directive
    */
    function originStoreSitestripeTimermessageoftheday(ComponentsConfigFactory, GamesDataFactory) {
        /**
        * The directive link
        */
        function originStoreSitestripeTimermessageofthedayLink(scope, elem) {
            // Add Scope functions and call the controller from here
        	
        	elem.find('.origin-store-sitestripe-messageoftheday-close').on('click', function(){
				elem.css('height', 0);
				
				localStorage.omotd = 'z';
			});
        	
        	if(scope.offerid) {
	            GamesDataFactory.getCatalogInfo([scope.offerid])
	                .then(function(){
                        /* This will come from OCD Feed */
                        scope.href = "/#/storepdp";
                        
                        // This will be replaced with release date
                        scope.goaldate = "June 19, 2015 01:30:00 PM PDT"; 
                        scope.$digest();
	                })
	                .catch(function(error) {
	                    Origin.log.exception(error, 'origin-store-home-promo-timerfeatured - getCatalogInfo');
	                });
        	}
        }
        return {
            restrict: 'E',
            replace: true,
            scope: {
            	title: '@',
            	subtitle: '@',
            	offerid: '@',
            	image: '@',
            	logo: '@',
            	href: '@',
            	edgecolor: '@',
            	hidetimer: '@',
        	    goaldate: '@',
        	    newtitle: '@',
        	    newsubtitle: '@',
        	    timerposition: '@'
            },
            controller: 'OriginStoreSitestripeTimermessageofthedayCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('/store/sitestripe/views/messageoftheday.html'),
            link: originStoreSitestripeTimermessageofthedayLink
        };
    }
    
    /* jshint ignore:start */
    
    /**
     * Sets the layout of timer in this site stripe.
     * @readonly
     * @enum {string}
     */
    var TimerPositionEnumeration = {
        "Left": "left",
        "Right": "right"
    };
    
    /**
     * Hide the timer when it expires.
     * @readonly
     * @enum {string}
     */
    var HideEnumeration = {
        "True": true,
        "False": false
    };
    
    /**
     * ExpiredDateTime display format
     * @see http://docs.adobe.com/docs/en/cq/5-6/widgets-api/index.html?class=Date
     */
   
    var ExpiredDateTime = {
        "format": "F j, Y H:i:s T"
    };
    
    /* jshint ignore:end */
    
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreSitestripeTimermessageoftheday
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} title The title for this site stripe.
     * @param {LocalizedString} subtitle The title for this site stripe.
     * @param {string} offerid The id of the offer associated with this site stripe (Optional).
     * @param {ImageUrl|OCD} image The background image for this site stripe.
     * @param {ImageUrl|OCD} logo The logo image for this site stripe.
     * @param {string} href The URL for this site stripe to link to.
     * @param {string} edgecolor The background color for this site stripe (Hex: #000000).
     * @param {HideEnumeration} hidetimer Hide the timer when it expires?
     * @param {ExpiredDateTime} goaldate The timer end date and time.
     * @param {LocalizedString} newtitle The new title to switch to after the timer expires (Optional).
     * @param {LocalizedString} newsubtitle The new subtitle to switch to after the timer expires (Optional).
     * @param {TimerPositionEnumeration} timerposition Sets the layout of the timer.
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-sitestripe-timermessageoftheday
     *     		title="some title"
     *     		subtitle="some subtitle"
     *     		offerid="OFR:45563456"
     *     		image="//somedomain/someimage.jpg"
     *     		logo="//somedomain.com/someimage.jpg"
     *     		href="//somedomain.com"
     *     		edgecolor="#000000"
     *     		hidetimer="true"
     *       	goaldate="2015-06-02 09:10:30 UTC"
     *          newtitle="some new title"
     *          newsubtitle="some new subtitle"
     *          timerposition="left">
     *     </origin-store-sitestripe-timermessageoftheday>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreSitestripeTimermessageofthedayCtrl', OriginStoreSitestripeTimermessageofthedayCtrl)
        .directive('originStoreSitestripeTimermessageoftheday', originStoreSitestripeTimermessageoftheday);
}()); 
