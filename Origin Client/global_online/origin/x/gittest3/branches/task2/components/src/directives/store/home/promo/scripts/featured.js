
/**
 * @file /store/home/promo/scripts/featured.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-home-promo-featured';

    function OriginStoreHomePromoFeaturedCtrl() {

    	this.transfromDate = function(dateString) {
            var date = new Date(dateString);
            return date.toLocaleDateString();
        };
    }

    /**
    * The directive
    */
    function originStoreHomePromoFeatured(ComponentsConfigFactory, GamesDataFactory, UtilFactory) {
        /**
        * The directive link
        */
        function originStoreHomePromoFeaturedLink(scope, elem, attrs, ctrl) {
            // Add Scope functions and call the controller from here

        	elem.css('background-color', scope.edgecolor);

        	if(scope.offerid) {
	            GamesDataFactory.getCatalogInfo([scope.offerid])
	                .then(function(data){
                        /* This will come from OCD Feed */
                        scope.title = data[scope.offerid].displayName;
                        scope.href = "/#/storepdp";

                        /* This will come from rating */
                        scope.price = '$59.99';
                        scope.strike = '$79.99';
                        scope.saleString = "Save 33%";
                        scope.$digest();
	                })
                    .catch(function(error) {
	                    Origin.log.exception(error, 'origin-store-home-promo-featured - getCatalogInfo');
	                });
        	}

        	if(scope.available && scope.enddate) {
	        	scope.eDate = ctrl.transfromDate(scope.enddate);
	        	scope.aText = UtilFactory.getLocalizedStr(scope.available, CONTEXT_NAMESPACE, 'releaseString', {'%endDate%': scope.eDate});
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
            	available: '@',
            	enddate: '@',
            	header: '@'
            },
            controller: OriginStoreHomePromoFeaturedCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('/store/home/promo/views/featured.html'),
            link: originStoreHomePromoFeaturedLink
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

	    var ExpiredDateTime = {
	        "format" : "F j Y H:i:s T"
	    };

    /* jshint ignore:end */

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreHomePromoFeatured
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
     * @param {LocalizedString} bulletone The first bullet point.
     * @param {LocalizedString} bullettwo The second bullet point.
     * @param {ColumnWidthEnumeration} size Sets the layout of this module and its responsive behaviour.
     * @param {string} edgecolor The color to bleed into at the edge of the background image.
     * @param {string} href The url to link this promo to. Overrides pdp url if set.
     * @param {ExpiredDateTime} enddate The expiry date/time for the availability text.
     * @param {LocalizedString} available The string that holds the end date. The date will be subbed in the place of %endDate%. EG: "Available Until %endDate%"
     * @param {LocalizedString} header The header for this promo (Optional).
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-home-promo-featured
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
     *       	enddate="March 23 2015 10:15:00 PM PST"
     *       	available="Available Until %endDate%">
     *     </origin-store-home-promo-featured>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
    	.controller('OriginStoreHomePromoFeaturedCtrl', OriginStoreHomePromoFeaturedCtrl)
        .directive('originStoreHomePromoFeatured', originStoreHomePromoFeatured);
}());
