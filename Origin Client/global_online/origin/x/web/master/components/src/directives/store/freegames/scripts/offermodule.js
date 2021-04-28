/**
 * @file store/freegames/scripts/offermodule.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-freegames-module';

    function OriginStoreFreegamesOffermoduleCtrl($scope, UtilFactory) {
        $scope.offerbtndescription = UtilFactory.getLocalizedStr($scope.offerbtndescription, CONTEXT_NAMESPACE, 'offerGetItNow');
    }

    function originStoreFreegamesOffermodule(ComponentsConfigFactory, GamesDataFactory, ComponentsLogFactory) {

        function originStoreFreegamesOffermoduleLink(scope) {
            // Add Scope functions and call the controller from here

            if(scope.offerid) {
                GamesDataFactory.getCatalogInfo([scope.offerid])
                    .then(function(data){
                        /* This will come from OCD Feed */
                        scope.offertitle = data[scope.offerid].i18n.displayName;
                        scope.offerhref = scope.href ? scope.href : "/#/storepdp";
                        scope.packart = data[scope.offerid].i18n.packArtLarge;
                        scope.$digest();
                    })
                    .catch(function(error) {
                        ComponentsLogFactory.error('origin-store-freegames-module - getCatalogInfo', error);
                    });
            }
        }

        return {
            restrict: 'E',
            scope: {
                alignment: '@',
                title: '@',
                subtitle: '@',
                href: '@',
                description: '@',
                offerid: '@',
                offersubtitle: '@',
                offerdescription: '@',
                image: '@',
                startColor: '@',
                endColor: '@'
            },
            controller: 'OriginStoreFreegamesOffermoduleCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/freegames/views/offermodule.html'),
            link: originStoreFreegamesOffermoduleLink
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
     * @name origin-components.directives:originStoreFreegamesOffermodule
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {AlignmentEnumeration} alignment The text alignment of this module.
     * @param {LocalizedString} title The title for this module.
     * @param {string} href The URL for button below the text.
     * @param {LocalizedString} description The text for button below the text.
     * @param {string} offerid The id of the offer associated with this module.
     * @param {ImageUrl|OCD} image The background image for this module.
     * @param {LocalizedString} offersubtitle The subtitle in the offer section.
     * @param {LocalizedString} offerdescription The description in the offer section.
     * @param {string} start-color The start of the fade color for the background
     * @param {string} end-color The end color/background color of the module
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-freegames-offermodule
     *          alignment="left"
     *          title="Demos & Betas"
     *          subtitle="Experience a game before it’s released, or get a sample of the final game. Betas and demos are a great way to see what the games are all about."
     *          href="#/free-games/demos-betas"
     *          description="Learn More"
     *          offerid="Origin.OFR.50.0000462"
     *          image="http://docs.x.origin.com/origin-x-design-prototype/images/backgrounds/fifa.jpg"
     *          offersubtitle="Play for 48 free hours."
     *          offerdescriptiontitle="Download the FIFA 15 Demo"
     *          offerdescription="Feel the game. Try the Ignite Engine for free.">
     *      </origin-store-freegames-offermodule>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreFreegamesOffermoduleCtrl', OriginStoreFreegamesOffermoduleCtrl)
        .directive('originStoreFreegamesOffermodule', originStoreFreegamesOffermodule);
}());
