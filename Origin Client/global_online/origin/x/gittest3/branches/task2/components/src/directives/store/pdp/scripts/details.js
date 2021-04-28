
/** 
 * @file store/pdp/scripts/details.js
 */ 
(function(){
    'use strict';
    /**
    * The directive
    */
    function originStorePdpDetails(ComponentsConfigFactory, GamesDataFactory, $stateParams) {
        /**
        * The directive link
        */
        function originStorePdpDetailsLink(scope) {
            // TODO: will need to fetch catalog info based on offer path, rather than offerid
            if($stateParams.offerid) {
                GamesDataFactory.getCatalogInfo([$stateParams.offerid])
                    .then(function(data){
                        if (data){
                            scope.$evalAsync(function() {
                                // TODO: replace with merchandized feature
                                // directive.  Blocked by merchandizing
                                // ORSTOR-894

                                // icon+features
                                scope.features = [
                                    {icon: "otkicon-originlogo", title: '<a class="otka" href="#">Origin Great Guarantee Eligible</a>'},
                                    {icon: "otkicon-person", title: "Single Player"},
                                    {icon: "otkicon-addcontact", title: "Multiplayer"},
                                    {icon: "otkicon-cloud", title: "Origin Cloud Sync Enabled"},
                                    {icon: "otkicon-controller", title: "Full Controller Support"},
                                    {icon: "otkicon-trophy", title: "Origin Achievements Enabled"}
                                ];
                            });
                        }
                    })
                    .catch(function(error) {
                        Origin.log.exception(error, 'origin-store-pdp-details - getCatalogInfo');
                    });
            }
        }
        return {
            restrict: 'E',
            scope: {},
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/details.html'),
            link: originStorePdpDetailsLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpDetails
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @description PDP details blocks, retrieved from catalog
     *
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-details></origin-store-pdp-details>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .directive('originStorePdpDetails', originStorePdpDetails);
}()); 
