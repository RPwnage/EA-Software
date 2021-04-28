
/** 
 * @file store/freegames/scripts/program.js
 */ 
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-freegames-program';

    /**
    * The controller
    */
    function OriginStoreFreegamesProgramCtrl($scope, UtilFactory) {
        
        $scope.description = UtilFactory.getLocalizedStr($scope.offerbtndescription, CONTEXT_NAMESPACE, 'offerGetItNow');

    }
    /**
    * The directive
    */
    function originStoreFreegamesProgram(ComponentsConfigFactory, GamesDataFactory) {
        /**
        * The directive link
        */
        function originStoreFreegamesProgramLink(scope, elem) {
            // Add Scope functions and call the controller from here

            if(scope.offerid) {
                GamesDataFactory.getCatalogInfo([scope.offerid])
                    .then(function(data){
                        scope.$evalAsync(function() {
                            /* This will come from OCD Feed */
                            scope.offertitle = data[scope.offerid].displayName;
                            scope.offerhref = '/#/storepdp';
                            scope.entitlmenturl = '#/free-games/on-the-house';
                            scope.packart = data[scope.offerid].packArt;
                            scope.offerprice = 'FREE';
                            scope.offeroriginal = '$79.99';

                            elem.removeClass('origin-store-freegames-program-loading');
                        });
                    })
                    .catch(function(error) {
                        Origin.log.exception(error, 'origin-store-freegames-module - getCatalogInfo');
                    });
            }
        }
        return {
            restrict: 'E',
            replace: true,
            scope: {
                size: '@',
                description: '@',
                offerid: '@',
                image: '@',
                offerdescriptiontitle: '@',
                offerdescription: '@'
            },
            controller: 'OriginStoreFreegamesProgramCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/freegames/views/program.html'),
            link: originStoreFreegamesProgramLink
        };
    }

    /* jshint ignore:start */
    
        /**
         * Sets the layout of this module.
         * @readonly
         * @enum {string}
         */
        var SizeEnumeration = {
            "Full": "full",
            "Half": "half"
        };
    
    /* jshint ignore:end */

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreFreegamesProgram
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {SizeEnumeration} size The horizontal width of this module in its row.
     * @param {LocalizedString} description The text for button (Optional).
     * @param {string} offerid The id of the offer associated with this module.
     * @param {ImageUrl} image The background image for this module.
     * @param {LocalizedString} offerdescriptiontitle The description title in the offer section.
     * @param {LocalizedString} offerdescription The description in the offer section.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-store-freegames-program
     *              size="half"
     *              description="Learn More"
     *              offerid="OFB-EAST:109552419"
     *              image="http://docs.x.origin.com/origin-x-design-prototype/images/backgrounds/ultima.jpg"
     *              offerdescriptiontitle="What kind of city will you create?"
     *              offerdescription="You are an architect, cityplanner, Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Nulla vitae elit libero, a pharetra augue.">
     *          </origin-store-freegames-program>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreFreegamesProgramCtrl', OriginStoreFreegamesProgramCtrl)
        .directive('originStoreFreegamesProgram', originStoreFreegamesProgram);
}()); 
