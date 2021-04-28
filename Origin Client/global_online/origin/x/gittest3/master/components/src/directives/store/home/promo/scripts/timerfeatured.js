
/** 
 * @file /store/home/promo/scripts/timerfeatured.js
 */ 
(function(){
    'use strict';
    /**
     * The Controller
     */
    function OriginStoreHomePromoTimerfeaturedCtrl($scope) {
    	
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
    function originStoreHomePromoTimerfeatured(ComponentsConfigFactory, GamesDataFactory) {
        /**
        * The directive link
        */
        function originStoreHomePromoTimerfeaturedLink(scope, elem) {
            // Add Scope functions and call the controller from here
     
        	elem.css('background-color', scope.edgecolor);

        	if(scope.offerid) {
	            GamesDataFactory.getCatalogInfo([scope.offerid])
	                .then(function(data){
                        /* This will come from OCD Feed */
                        scope.title = data[scope.offerid].displayName;
                        scope.href = "/#/storepdp";
                        
                        // This will be replaced with release date
                        scope.goaldate = "June 19, 2015 01:30:00 PM PDT"; 

                        /* This will come from rating */
                        scope.price = '$59.99';
                        scope.strike = '$79.99';
                        scope.saleString = "Save 33%";
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
            transclude: true,
            scope: {
            	title: '@',
            	subtitle: '@',
            	offerid: '@',
            	videoid: '@',
                restrictedage: '@',
            	image: '@',
            	bulletone: '@',
            	bullettwo: '@',
            	size: '@',
            	edgecolor: '@',
            	href: '@',
            	goaldate: '@',
            	hidetimer: '@',
            	newtitle: '@',
            	newsubtitle: '@',
            	header: '@'
            },
            controller: OriginStoreHomePromoTimerfeaturedCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('/store/home/promo/views/featured.html'),
            link: originStoreHomePromoTimerfeaturedLink
        };
    }
    
    /* jshint ignore:start */
    
    /**
     * Sets the layout of this module and its responsive behaviour.
     * @readonly
     * @enum {string}
     */
    var ColumnWidthEnumeration = {
        "Full": "full",
        "Half": "half",
        "Third": "third"
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
     * @name origin-components.directives:originStoreHomePromoTimerfeatured
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} title The title for this promo.
     * @param {LocalizedString} subtitle The subtitle for this promo.
     * @param {string} offerid The id of the offer associated with this promo (Optional).
     * @param {string} videoid The youtube video ID of the video for this promo (Optional).
     * @param {string} restrictedage The Restricted Age of the youtube video for this promo (Optional).
     * @param {ImageUrl|OCD} image The background image for this promo.
     * @param {LocalizedString} bulletone The first bullet point (Requires Offer ID).
     * @param {LocalizedString} bullettwo The second bullet point (Requires Offer ID).
     * @param {ColumnWidthEnumeration} size Sets the layout of this module and its responsive behaviour.
     * @param {string} edgecolor The color to bleed into at the edge of the background image.
     * @param {string} href The url to link this promo to. Overrides pdp url if set.
     * @param {HideEnumeration} hidetimer Hide the timer when it expires?
     * @param {ExpiredDateTime} goaldate The timer end date and time.
     * @param {LocalizedString} newtitle The new title to switch to after the timer expires (Optional).
     * @param {LocalizedString} newsubtitle The new subtitle to switch to after the timer expires (Optional).
     * @param {LocalizedString} header The header for this promo (Optional).
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-home-promo-timerfeatured
     *     		title="Some Title"
     *       	subtitle="Some Subtitle"
     *       	offerid="OFR:1234456"
     *       	videoid="j573kf7de"
     *          restrictedage = "18"
     *       	image="//somedomain/someimage.jpg"
     *       	bulletone="some bullet point"
     *       	bullettwo="some other bullet point"
     *       	size="full|half|third"
     *       	edgecolor="#000000"
     *       	href="/someurl.com"
     *       	hidetimer="true"
     *       	goaldate="2015-06-02 09:10:30 UTC"
     *          newtitle="some new title"
     *          newsubtitle="some new subtitle">
     *     </origin-store-home-promo-timerfeatured>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
    	.controller('OriginStoreHomePromoTimerfeaturedCtrl', OriginStoreHomePromoTimerfeaturedCtrl)
        .directive('originStoreHomePromoTimerfeatured', originStoreHomePromoTimerfeatured);
}()); 
