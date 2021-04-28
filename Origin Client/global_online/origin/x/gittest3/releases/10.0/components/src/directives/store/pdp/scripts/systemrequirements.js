
/** 
 * @file store/pdp/scripts/systemrequirements.js
 */ 
(function(){
    'use strict';

    /**
    * The directive
    */
    function originStorePdpSystemrequirements(ComponentsConfigFactory, GamesDataFactory, $stateParams) {
        /**
        * The directive link
        */
        function originStorePdpSystemrequirementsLink(scope) {
            // TODO: will need to fetch catalog info based on offer path, rather than offerid
            if($stateParams.offerid) {
                GamesDataFactory.getCatalogInfo([$stateParams.offerid])
                    .then(function(data){
                        if (data){
                            scope.$evalAsync(function() {
                                // TODO: replace with catalog data
                                scope.systemrequirements = '<b>Minimum System Requirements</b><br>• OS: Windows Vista SP2 32-bit<br>• Processor (AMD): Athlon X2 2.8 GHz<br>• Processor (Intel): Core 2 Duo 2.4 GHz<br>• Memory: 4 GB<br>• Hard Drive: 30 GB<br>• Graphics card (AMD): AMD Radeon HD 3870<br>• Graphics card (NVIDIA): NVIDIA GeForce 8800 GT<br>• Graphics memory: 512 MB<br><br><b>Recommended System Requirements</b><br>• OS: Windows 8 64-bit<br>• Processor (AMD): Six-core CPU<br>• Processor (Intel): Quad-core CPU<br>• Memory: 8 GB<br>• Hard Drive: 30 GB<br>• Graphics card (AMD): AMD Radeon HD 7870<br>• Graphics card (NVIDIA): NVIDIA GeForce GTX 660<br>• Graphics memory: 3 GB';
                            });
                        }
                    })
                    .catch(function(error) {
                        Origin.log.exception(error, 'origin-store-pdp-systemrequirements - getCatalogInfo');
                    });
            }
        }
        return {
            restrict: 'E',
            scope: {},
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/systemrequirements.html'),
            link: originStorePdpSystemrequirementsLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpSystemrequirements
     * @restrict E
     * @element ANY
     * @scope
     * @description PDP system requirements block, retrieved from catalog
     *
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-systemrequirements></origin-store-pdp-systemrequirements>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .directive('originStorePdpSystemrequirements', originStorePdpSystemrequirements);
}()); 
