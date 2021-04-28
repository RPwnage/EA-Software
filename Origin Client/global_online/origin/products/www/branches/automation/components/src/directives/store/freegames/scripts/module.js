
/**
 * @file store/freegames/scripts/module.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-freegames-module',
        MODULE_SELECTOR = '.origin-store-freegames-module';

    /**
    * The controller
    */
    function OriginStoreFreegamesModuleCtrl($scope, UtilFactory) {

    	$scope.description = UtilFactory.getLocalizedStr($scope.description, CONTEXT_NAMESPACE, 'description');

    }
    /**
    * The directive
    */
    function originStoreFreegamesModule(ComponentsConfigFactory, CSSUtilFactory) {

        function originStoreFreegamesModuleLink (scope, element) {
                scope.backgroundColor = CSSUtilFactory.setBackgroundColorOfElement(scope.backgroundColor, element.find(MODULE_SELECTOR), CONTEXT_NAMESPACE);
        }

        return {
            restrict: 'E',
            scope: {
            	alignment: '@',
            	titleStr: '@',
                subtitle: '@',
                href: '@',
                description: '@',
                image: '@',
                offersubtitle: '@',
                offerdescriptiontitle: '@',
                offerdescription: '@',
                backgroundColor: '@'
            },
            controller: 'OriginStoreFreegamesModuleCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/freegames/views/module.html'),
            link: originStoreFreegamesModuleLink
        };
    }

    /* jshint ignore:start */

	    /**
	     * Sets the layout of this module.
	     * @readonly
	     * @enum {string}
	     */
	    var AlignmentEnumeration = {
	        "Left": "left",
	        "Right": "right"
	    };

    /* jshint ignore:end */

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreFreegamesModule
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {AlignmentEnumeration} alignment The text alignment of this module.
     * @param {LocalizedString} title-str The title for this module.
     * @param {LocalizedString} subtitle The title for this module.
     * @param {string} href The URL for button below the text.
     * @param {LocalizedString} description The text for button below the text.
     * @param {ImageUrl|OCD} image The background image for this module.
     * @param {string} background-color Hex value of the background color
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     		<origin-store-freegames-module
	 *		        alignment="left"
	 *		        title-str="Demos & Betas"
	 *		        subtitle="Experience a game before it’s released, or get a sample of the final game. Betas and demos are a great way to see what the games are all about."
	 *		        href="free-games/demos-betas"
	 *		        description="Learn More"
	 *		        image="https://docs.x.origin.com/proto/images/backgrounds/fifa.jpg">
	 *		    </origin-store-freegames-module>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreFreegamesModuleCtrl', OriginStoreFreegamesModuleCtrl)
        .directive('originStoreFreegamesModule', originStoreFreegamesModule);
}());
