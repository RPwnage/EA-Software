/**
 * @file home/discovery/tile/programmablenews.js
 */
(function() {
    'use strict';

    function originHomeDiscoveryTileProgrammableNews(ComponentsConfigFactory) {

        function originHomeDiscoverTileProgrammableNewsLink(scope, elem, attr, ctrl, $transclude) {
            $transclude(function(clone) {
                //if there are no elements passed in for transclude it means we have no buttons
                //so don't show the hover
                scope.showHover = clone.length;
            });

            // authorDetails will contain either a creationdate or authordescription. If both are present then creationdate will take precedence
            scope.authorDetails  = scope.creationdate ? moment(scope.creationdate).format('LL') : scope.authordescription;
            scope.authorVisible = scope.authorname || (scope.authorhref && scope.authorname) || scope.creationdate || scope.authordescription || scope.timetoread;

            scope.onAuthorClicked = function() {
                if (scope.authorhref) {
                    Origin.windows.externalUrl(scope.authorhref);
                }
            };
        }

        return {
            restrict: 'E',
            transclude: true,
            replace: true,
            scope: {
                image: '@',
                descriptiontitle: '@',
                description: '@',
                authorname: '@',
                authordescription: '@',
                authorhref: '@',
                creationdate: '@',
                timetoread: '@',
                recoid:'@',
                recoindex:'@',
                recotag:'@',
                recotype: '@',
                ctid: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/discovery/tile/views/programmableactionnews.html'),
            controller:'OriginHomeDiscoveryTileProgrammableActionNewsCtrl',
            link: originHomeDiscoverTileProgrammableNewsLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileProgrammableNews
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} ctid the Content Tracking ID used for telemetry
     * @param {ImageUrl} image the image to use
     * @param {LocalizedText} descriptiontitle the news header
     * @param {LocalizedText} description the description text
     * @param {LocalizedString} authorname an optional author name eg. JohnDoe253
     * @param {LocalizedString} authordescription an optional author description eg. Youtube User
     * @param {LocalizedString} timetoread an optional time to read/watch description eg. 4 min read
     * @param {Url} authorhref an optional author external link
     * @param {DateTime} creationdate date when this tile is created
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
     *          <origin-home-discovery-tile-config-programmable-news image="https://data1.origin.com/live/content/dam/originx/web/app/programs/MyNewsCommunity/Sep/TF2_Pilot_Trailer_Logo.jpg" descriptiontitle="EXPLORE SIMCITY BUILDIT" description="Welcome, Mayors! Three weeks ago, our Helsinki studio released SimCity BuildIt - an all-new version of SimCity for a new generation of players on smartphones and tablets." placementtype="position" placementvalue="1" authorname="bc" authordescription="description for bc" authorhref="http://www.espn.com" timetoread="4 minutes">
     *               <origin-home-discovery-tile-config-cta actionid="url-external" href="http://www.ea.com/simcity-buildit/" description="Create Your SimCity" type="primary"></origin-home-discovery-tile-config-cta>
    *            </origin-home-discovery-tile-config-programmable-news>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originHomeDiscoveryTileProgrammableNews', originHomeDiscoveryTileProgrammableNews);
}());