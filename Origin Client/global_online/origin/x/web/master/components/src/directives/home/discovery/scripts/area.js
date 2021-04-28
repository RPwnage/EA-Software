/**
 * @file home/discovery/area.js
 */
(function() {
    'use strict';

    function OriginHomeDiscoveryAreaCtrl(DiscoveryStoryFactory) {
        //TODO: look into cleaning this up.
        DiscoveryStoryFactory.reset();
    }

    function originHomeDiscoveryArea($compile, OcdHelperFactory, GamesDataFactory, ProgrammedIntroductionFactory, ComponentsConfigFactory, ComponentsLogFactory) {
        function originHomeDiscoveryAreaLink(scope, element) {
            /**
             * GEO can set up game introduction tiles that are based on entitlement and OCD details
             * It is used to introduce a user to new content he's acquired. This information is appended
             * to a div that is above normally merchandised programmable tiles. Programmable tiles in this
             * zone must be configured with a priority of 1 to display.
             */
            function buildProgrammedIntroductionTiles() {
                /**
                 * Given a list of introduction tiles, return the dom elements for a discover bucket
                 * @param  {Object[]} introductionTiles The applicable game introduction tiles
                 * @return {HtmlElement[]} the introduction tile buckets in OCD format
                 */
                function createDiscoveryBucketsFromIntroductionTiles(introductionTiles) {
                    return introductionTiles.map(function(tile) {
                        return OcdHelperFactory.slingDataToDom({
                            'origin-home-discovery-bucket': {
                                'title': tile.title,
                                'items': tile.items
                            }
                        });
                    });
                }

                /**
                 * Append the introduction bucket DOM to its container
                 * @param  {Object.<HTMLElement>[]} templates The bucket HTMLElement(s)
                 * @return {Object.<HTMLElement>[]} The bucketHTMLElement(s)
                 */
                function compileBucket(templates) {
                    var introTileElement = element.find('.programmed-introduction-tiles');

                    for(var i = 0; i < templates.length; i++) {
                        introTileElement.append(
                            $compile(templates[i])(scope)
                        );
                    }

                    return templates;
                }

                ProgrammedIntroductionFactory.getTileData()
                    .then(createDiscoveryBucketsFromIntroductionTiles)
                    .then(compileBucket)
                    .then(function() {
                        scope.$digest();
                    })
                    .catch(function(error){
                        ComponentsLogFactory.error('ERROR: Could not resolve programmed introduction tiles with error: ' + error);
                    });
            }

            GamesDataFactory.events.on('games:baseGameEntitlementsUpdate', buildProgrammedIntroductionTiles);
            if (GamesDataFactory.initialRefreshCompleted()) {
                buildProgrammedIntroductionTiles();
            }

            scope.$on('$destroy', function() {
                GamesDataFactory.events.off('games:baseGameEntitlementsUpdate', buildProgrammedIntroductionTiles);
            });
        }

        return {
            restrict: 'E',
            transclude: true,
            controller: 'OriginHomeDiscoveryAreaCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/discovery/views/area.html'),
            link: originHomeDiscoveryAreaLink
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
     *             <origin-home-discovery-bucket titlestr="Discover Something New!">
     *                 <origin-home-discovery-tile-config-recfriends limit="2" diminish="500" priority="2500"></origin-home-discovery-tile-config-recfriends>
     *                 <origin-home-discovery-tile-config-newdlc limit="2" diminish="500" priority="2100"></origin-home-discovery-tile-config-newdlc>
     *             </origin-home-discovery-bucket>
     *             <origin-home-discovery-bucket titlestr="What Your Friends are Doing">
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
        .controller('OriginHomeDiscoveryAreaCtrl', OriginHomeDiscoveryAreaCtrl)
        .directive('originHomeDiscoveryArea', originHomeDiscoveryArea);
}());