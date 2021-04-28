/**
 * @file home/discovery/tile/programmableaction.js
 */
(function() {
    'use strict';

    function originHomeDiscoveryTileProgrammableAction(ComponentsConfigFactory) {

        function originHomeDiscoverTileProgrammableActionLink(scope, elem, attr, ctrl, $transclude) {
            $transclude(function(clone) {
                //if there are no elements passed in for transclude it means we have no buttons
                //so don't show the hover
                scope.showHover = clone.length;
            });
        }

        return {
            restrict: 'E',
            transclude: true,
            replace: true,
            scope: {
                image: '@',
                descriptiontitle: '@',
                description: '@',
                ctid: '@',
                recoid:'@',
                recoindex:'@',
                recotag:'@',
                recotype: '@'
            },
            controller: 'OriginHomeDiscoveryTileProgrammableActionNewsCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/discovery/tile/views/programmableactionnews.html'),
            link: originHomeDiscoverTileProgrammableActionLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileProgrammableAction
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} ctid the Content Tracking ID used for telemetry
     * @param {ImageUrl} image the image to use
     * @param {LocalizedText} descriptiontitle the news header
     * @param {LocalizedText} description the description text
     * @param {string} recotag a tag returned by the recommendation API that we need to send for tracking
     * @param {string} recoindex the index of where the tile sits in the bucket
     * @param {string} recoid the unique id of the tile (ctid for news/ project id for games)
     * @param {string} recotype the type of the recommendation (news/games)

     * @description
     *
     * programmable discovery tile
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-home-discovery-tile-config-programmable-action image="https://data1.origin.com/live/content/dam/originx/web/app/programs/MyNewsCommunity/Sep/TF2_Pilot_Trailer_Logo.jpg" descriptiontitle="action test" description="This is a long description about action stuff." placementtype="position" placementvalue="1" authorname="bc" authordescription="description for bc" authorhref="http://www.espn.com" timetoread="4 minutes">
     *             <origin-home-discovery-tile-config-cta actionid="url-external" href="http://www.ea.com/simcity-buildit/" description="Open External" type="primary"></origin-home-discovery-tile-config-cta>
     *          </origin-home-discovery-tile-config-programmable-action>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originHomeDiscoveryTileProgrammableAction', originHomeDiscoveryTileProgrammableAction);
}());