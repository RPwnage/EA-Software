/**
 * @file home/discovery/tile/programmable.js
 */
(function() {
    'use strict';

    /* jshint ignore:start */
    var FriendsListEnumeration = {
        "friendsPlayedOrPlaying": "friendsPlayedOrPlaying",
        "friendsOwned": "friendsOwned"
    };
    /* jshint ignore:end */

    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };

    var CONTEXT_NAMESPACE = 'origin-home-discovery-tile-programmable';

    function OriginHomeDiscoveryTileProgrammableCtrl($scope, $sce, $timeout, UtilFactory, ProductFactory, PdpUrlFactory, OgdUrlFactory, GamesDataFactory, ComponentsLogFactory) {
 
        var friendsTextVisible = false,
            friendsPlayingTextVisible = false,
            friendsTileDisplayVisible = false,
            TELEMETRYPREFIX = 'telemetry-',
            friendsSubtitleVisible = false;

        var throttledDigestFunction = UtilFactory.throttle(function() {
            $timeout(function() {
                $scope.$digest();
            }, 0, false);
        }, 1000);

        //this string will already be localized as it must always come from CQ5
        $scope.description = $sce.trustAsHtml($scope.descriptionRaw);

        /**
         *  build the URL for the pdp page associated with the attached product model
         * @return {void}
         */
        $scope.goToPdp = function() {
            if ($scope.model) {
                PdpUrlFactory.goToPdp($scope.model);
            }
        };

        /**
         * Link to the selected game
         */
        $scope.navigateToGame = function() {
            if ($scope.offerId) {
                if(GamesDataFactory.getEntitlement($scope.offerId)) {
                    OgdUrlFactory.goToOgd($scope.model);
                } else {
                    PdpUrlFactory.goToPdp($scope.model);
                }
            }
        };

        /**
         * triggered when clicking on the author link
         */
        $scope.onAuthorClicked = function() {
            Origin.windows.externalUrl($scope.authorhref);
        };

        /**
         * checks if price should be visible
         * @return {Boolean} true if visible
         */
        $scope.isPriceVisible = function() {
            return String($scope.showPrice).toLowerCase() === BooleanEnumeration.true;
        };

        function setModelAndBindings(offerId) {
            ProductFactory.get(offerId).attachToScope($scope, function(model) {
                $scope.model = model;
                setGameNameAndImage();
                $scope.offerId = model.offerId;
            });
        }

        function setGameNameAndImage() {
            $scope.gamename = $scope.gamename || _.get($scope.model, 'displayName');

            //if we have catalog info but no image, populate it with box art as fallback
            if(!$scope.image) {
                $scope.usingPackArt = true;
                $scope.image = _.get($scope.model, 'packArt');
                $scope.imageStyle = {};
            } else {
                $scope.imageStyle = {
                    'background-image' : 'url(' + $scope.image + ')'
                };
            }
        }

        function handleError(error) {
            ComponentsLogFactory.error('[origin-home-discovery-tile-programmable]', error);
        }

        /**
         * Send telemetry when the cta is clicked
         */
        function onCtaClicked(event, format) {
            var source = Origin.client.isEmbeddedBrowser() ? 'client' : 'web';
            format = format? format: 'live_tile';

            event.stopPropagation();
            Origin.telemetry.sendStandardPinEvent(Origin.telemetry.trackerTypes.TRACKER_MARKETING, 'message', source, 'success', {
                'type': 'in-game',
                'service': 'originx',
                'format': format,
                'client_state': 'foreground',
                'msg_id': $scope.ctid? $scope.ctid: $scope.image,
                'status': 'click',
                'content': $scope.descriptionRaw,
                'destination_name': _.get($scope, ['$parent', 'titleStr']) || ''
            });

        }

        function updateSupportTextVisible() {
            $scope.supportTextVisible = friendsTextVisible || friendsPlayingTextVisible || friendsTileDisplayVisible || friendsSubtitleVisible || $scope.isPriceVisible();
            throttledDigestFunction();
        }

        function setSupportTextVisible() {
            $scope.supportTextVisible = false;

            $scope.$on('friendsTextVisible', function(event, visible) {
                friendsTextVisible = visible;
                updateSupportTextVisible();
            });

            $scope.$on('friendsPlayingTextVisible', function(event, visible) {
                friendsPlayingTextVisible = visible;
                updateSupportTextVisible();
            });

            $scope.$on('friendsTileDisplayVisible', function(event, visible) {
                friendsTileDisplayVisible = visible;
                updateSupportTextVisible();
            });

            $scope.$on('friendsSubtitleVisible', function(event, visible) {
                friendsSubtitleVisible = visible;
                updateSupportTextVisible();
            });

            // check if the 'origin-tile-support-text' element should be shown
            updateSupportTextVisible();
        }

        function setAuthorVisible() {
            $scope.authorVisible = ($scope.authorhref && $scope.authorname) ||
                $scope.authordescription ||
                $scope.timetoread;
        }

        // We listen for an event that is emitted by the transcluded cta directive,
        // in order to send telemetry that includes the ctid, which identifies the tile.
        $scope.$on('cta:clicked', onCtaClicked);

        setSupportTextVisible();
        setAuthorVisible();

        //set a class in the dom as a marker for telemetry
        $scope.telemetryTrackerClass =  TELEMETRYPREFIX + CONTEXT_NAMESPACE;

        //send impression telemetry data once dom is loaded
        setTimeout(Origin.telemetry.sendDomLoadedTelemetryEvent, 0);

        GamesDataFactory.lookUpOfferIdIfNeeded($scope.ocdPath, $scope.offerId)
            .then(setModelAndBindings)
            .catch(handleError);
    }

    function originHomeDiscoveryTileProgrammable(ComponentsConfigFactory) {

        function originHomeDiscoverTileProgrammableLink(scope, elem, attr, ctrl, $transclude) {
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
                image: '@image',
                subtitle: '@subtitle',
                descriptionRaw: '@description',
                authorname: '@authorname',
                authordescription: '@authordescription',
                authorhref: '@authorhref',
                timetoread: '@',
                offerId: '@offerid',
                requestedFriendType: '@friendslisttype',
                friendsText: '@friendstext',
                friendsPopoutTitle: '@friendspopouttitle',
                showPrice: '@showprice',
                ocdPath: '@',
                ctid: '@',
                recotag: '@',
                recoindex: '@',
                recoid: '@',
                recotype: '@'

            },
            controller: 'OriginHomeDiscoveryTileProgrammableCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/discovery/tile/views/programmable.html'),
            link: originHomeDiscoverTileProgrammableLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileProgrammable
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} ctid the Content Tracking ID used for telemetry
     * @param {ImageUrl} image the image to use
     * @param {LocalizedText} subtitle the subtitle text
     * @param {LocalizedText} description the description text
     * @param {LocalizedString} authorname an optional author name eg. JohnDoe253
     * @param {LocalizedString} authordescription an optional author description eg. Youtube User
     * @param {LocalizedString} timetoread an optional time to read/watch description eg. 4 min read
     * @param {Url} authorhref an optional author external link
     * @param {string} offerid the offerid associated with the programmable tile
     * @param {OcdPath} ocd-path the path associated with the programmable tile
     * @param {FriendsListEnumeration} requestedfriendtype the requested friend type if there is one
     * @param {LocalizedText} friendstext the friends text associated with the avatar list
     * @param {LocalizedText} friendspopouttitle the title shown on the friends popout title
     * @param {BooleanEnumeration} showprice do we want to show a price if there is a valid offer id associated with the programmable tile
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
     *         <origin-home-discovery-tile-programmable image="/content/dam/originx/web/app/home/discovery/programmed/tile_add_friends_long.jpg" description="<strong>SIMS 4 GAME TIME</strong><br>Are you ready to rule? The Sims 4 is now available with Origin Game Time, giving players 48 hours of free, unrestricted gameplay.">
     *             <origin-cta-directacquisition actionid="direct-acquistion" href="pathtogamelibrary" description="Get It Now" type="primary"></origin-cta-directacquisition>
     *             <origin-cta-loadurlexternal actionid="url-external" href="https://www.origin.com/en-us/news/the-sims-4-game-time" description="Read More" type="secondary"></origin-cta-loadurlexternal>
     *         </origin-home-discovery-tile-programmable>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginHomeDiscoveryTileProgrammableCtrl', OriginHomeDiscoveryTileProgrammableCtrl)
        .directive('originHomeDiscoveryTileProgrammable', originHomeDiscoveryTileProgrammable);
}());