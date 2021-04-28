/**
 * @file home/discovery/introduction/introductionconfig.js
 */
(function() {

    'use strict';

    function OriginHomeDiscoveryTileIntroductionConfigCtrl($scope, GamesEntitlementFactory, DateHelperFactory) {
        /**
         * if the ocd path is an exact match or has a matching franchise or game
         * @param  {string} entitlementOfferPath the patch from the entitlement
         * @param  {string} pathToMatch          the path to match
         * @return {boolean}                     true if matches
         */
        function matchesOCDPath(entitlementOfferPath, pathToMatch) {
            return ((entitlementOfferPath === pathToMatch) || (_.startsWith(entitlementOfferPath, pathToMatch + '/')));
        }

        /**
         * determines whether to show the intro tile
         * @return {boolean} true if should show
         */
        $scope.shouldShowIntroTiles = function() {
            var entitlements = GamesEntitlementFactory.getAllEntitlements();

            //if nothing is passed in set a date of 30 days
            $scope.showfordays = parseInt($scope.showfordays) || 30;
            
            //if our offer starts with the ocd path entered and if we are with in the requested number of days
            //show the discover intro bucket
            //
            //returns once we find a match and early exits the loop
            if (_.isObject(entitlements)) {
                for (var offerId in entitlements) {
                    if (entitlements.hasOwnProperty(offerId) && matchesOCDPath(entitlements[offerId].offerPath, $scope.ocdPath) &&
                        DateHelperFactory.isInTheFuture(DateHelperFactory.addDays(entitlements[offerId].grantDate, $scope.showfordays))) {
                        return true;
                    }
                }
            }
            return false;
        };
    }

    function originHomeDiscoveryTileIntroductionConfig() {
        return {
            restrict: 'E',
            transclude: true,
            controller: 'OriginHomeDiscoveryTileIntroductionConfigCtrl',
            scope: {
                ocdPath: '@',
                showfordays: '@'
            },
            template: '<div ng-transclude ng-if="::shouldShowIntroTiles()"></div>'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileIntroductionConfig
     * @restrict E
     * @element ANY
     * @scope
     * @param {OcdPath} ocd-path - the ocd path used to compare against to determine whether to show the bucket
     * @param {Number} showfordays the number of days to show this tile after initial entitlement
     * @description
     *
     * This is an OCD configuration container for programmable tiles that introduces a newly entitled game
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-discovery-tile-introduction-config ocd-path="/dragon-age/dragon-age-inquisition" showfordays="1000">
     *             <origin-home-discovery-bucket title-str="Because you played BF4">
     *                  <origin-home-discovery-tile-config-programmable description="Battlefield Hardline is the most popular game this week" friendslisttype="friendsOwned" image="https://dev.data4.origin.com/preview/content/dam/originx/web/app/home/discovery/programmed/BFH_CriminalsCloseup.jpg" ocd-path="/battlefield/battlefield-hardline/digital-deluxe-edition" placementtype="priority" placementvalue="1200">
     *                      <origin-home-discovery-tile-config-cta actionid="pdp" description="Buy Me pls" ocd-path="/battlefield/battlefield-hardline/digital-deluxe-edition" type="primary"/>
     *                  </origin-home-discovery-tile-config-programmable>
     *             </origin-home-discovery-bucket>
     *          </origin-home-discovery-tile-introduction-config>
     *     </file>
     * </example> 
     */
    angular.module('origin-components')
        .controller('OriginHomeDiscoveryTileIntroductionConfigCtrl', OriginHomeDiscoveryTileIntroductionConfigCtrl)
        .directive('originHomeDiscoveryTileIntroductionConfig', originHomeDiscoveryTileIntroductionConfig);
}());