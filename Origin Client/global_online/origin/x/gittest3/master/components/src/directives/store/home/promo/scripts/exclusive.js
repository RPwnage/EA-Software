
/** 
 * @file /store/home/promo/scripts/exclusive.js
 */ 
(function(){
    'use strict';

    /**
    * The directive
    */
    function originStoreHomePromoExclusive(ComponentsConfigFactory, GamesDataFactory) {
        /**
        * The directive link
        */
        function originStoreHomePromoExclusiveLink(scope, elem) {
            // Add Scope functions and call the controller from here
        	
        	elem.css('background-color', scope.edgecolor);
        	
        	if(scope.offerid) {
	            GamesDataFactory.getCatalogInfo([scope.offerid])
	                .then(function(data){
	                        /* This will come from OCD Feed */
	                        scope.offertitle = data[scope.offerid].displayName;
                            scope.href = scope.href ? scope.href : '/#/storepdp';
	                        scope.packart = data[scope.offerid].packArt;
	
	                        /* This will come from rating */
	                        scope.price = '$59.99';
	                        scope.strike = '$79.99';
                            scope.saleString = 'Save 33%';
                        scope.$digest();
	                })
	                .catch(function(error) {
	                    Origin.log.exception(error, 'origin-store-home-promo-exclusive - getCatalogInfo');
	                });
        	}
        	
        }
        return {
            restrict: 'E',
            replace: true,
            transclude: true,
            scope: {
            	description: '@',
            	offerid: '@',
            	videoid: '@',
                restrictedage: '@',
            	image: '@',
            	bulletone: '@',
            	bullettwo: '@',
            	size: '@',
            	edgecolor: '@',
            	href: '@',
            	header: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('/store/home/promo/views/exclusive.html'),
            link: originStoreHomePromoExclusiveLink
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
    
    /* jshint ignore:end */
    
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreHomePromoExclusive
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} description The description for this promo.
     * @param {LocalizedString} subtitle The subtitle for this promo.
     * @param {string} offerid The id of the offer associated with this promo (Optional).
     * @param {string} videoid The youtube video ID of the video for this promo (Optional).
     * @param {string} restrictedage The Restricted Age of the youtube video for this promo (Optional).
     * @param {ImageUrl|OCD} img the pack art for the offer
     * @param {LocalizedString} bulletone The first bullet point.
     * @param {LocalizedString} bullettwo The second bullet point.
     * @param {ColumnWidthEnumeration} size Sets the layout of this module and its responsive behaviour.
     * @param {string} edgecolor The background color for this module.
     * @param {string} href The url this module links to.
     * @param {LocalizedString} header The header for this promo (Optional).
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-home-promo-exclusive
     *     		description="Some description"
     *       	subtitle="Some Subtitle"
     *       	offerid="OFR:1234456"
     *       	videoid="j573kf7de"
     *          restrictedage = "18"
     *       	image="//somedomain/someimage.jpg"
     *       	bulletone="some bullet point"
     *       	bullettwo="some other bullet point"
     *       	size="full|half|third"
     *       	edgecolor="#000000"
     *       	href="/someurl.com">
     *     </origin-store-home-promo-exclusive>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreHomePromoExclusive', originStoreHomePromoExclusive);
}()); 
