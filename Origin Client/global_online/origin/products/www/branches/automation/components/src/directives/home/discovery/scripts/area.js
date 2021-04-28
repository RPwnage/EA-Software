/**
 * @file home/discovery/area.js
 */
(function() {
    'use strict';

    function originHomeDiscoveryArea() {
        return {
            restrict: 'E',
            transclude: true,
            template: '<div ng-transclude></div>'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryArea
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * discovery area container (contains n-number of buckets)
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-discovery-area>
     *             <origin-home-discovery-bucket title-str="Discover Something New!">
     *                 <origin-home-discovery-tile-config-recfriends limit="2" diminish="500" priority="2500"></origin-home-discovery-tile-config-recfriends>
     *                 <origin-home-discovery-tile-config-newdlc limit="2" diminish="500" priority="2100"></origin-home-discovery-tile-config-newdlc>
     *             </origin-home-discovery-bucket>
     *             <origin-home-discovery-bucket title-str="What Your Friends are Doing">
     *                 <origin-home-discovery-tile-config-ugfriendsplaying limit="-1" diminish="500" priority="1400"></origin-home-discovery-tile-config-ugfriendsplaying>
     *                 <origin-home-discovery-tile-config-ogfriendsplaying limit="3" diminish="0" priority="3000"></origin-home-discovery-tile-config-ogfriendsplaying>
     *                 <origin-home-discovery-tile-config-programmable placementvalue="1" placementtype="priority" descriptionstr="Add more friends to see more content in this area." image="https://qa.www.assets.cms.origin.com/content/dam/originx/web/app/home/discovery/programmed/tile_add_friends_long.jpg">
     *                     <origin-home-discovery-tile-config-cta type="primary" href="/friendspage" descriptionstr="Learn More" actionid="url-internal"></origin-home-discovery-tile-config-cta>
     *                 </origin-home-discovery-tile-config-programmable>
     *             </origin-home-discovery-bucket>
     *         </origin-home-discovery-area>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originHomeDiscoveryArea', originHomeDiscoveryArea);
}());