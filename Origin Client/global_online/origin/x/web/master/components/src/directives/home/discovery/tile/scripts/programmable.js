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
    var ShowPriceEnumeration = {
        "true": "true",
        "false": "false"
    };

    function OriginHomeDiscoveryTileProgrammableCtrl($scope, $sce, $location, ComponentsUrlsFactory, UtilFactory, ProductFactory, DialogFactory, PdpUrlFactory) {
        //this string will already be localized as it must always come from CQ5
        $scope.description = $sce.trustAsHtml($scope.descriptionRaw);
        $scope.image = ComponentsUrlsFactory.getCMSImageUrl($scope.imageRaw);

        /*
         * Attach the product Model to the scope.
         */
        if ($scope.offerId) {
            // TODO Some tiles aren't getting any information about the product
            // this data needs to come from the feed so we can link to the product
            ProductFactory.get($scope.offerId).attachToScope($scope, 'model');
        }

        /**
         *  build the URL for the pdp page associated with the attached product model
         * @return {void}
         */
        $scope.goToPdp = function() {
            if ($scope.model) {
                PdpUrlFactory.goToPdp($scope.model);
            } else {
                DialogFactory.openAlert({
                    id: 'web-going-to-PDP',
                    title: 'Going to PDP',
                    description: 'When the rest of OriginX is fully functional, you will be taken to the PDP',
                    rejectText: 'OK'
                });
            }
        };

        $scope.isPriceVisible = function() {
            return String($scope.showPrice).toLowerCase() === ShowPriceEnumeration["true"];
        };
        /**
         * Check to see if the current device can handle touch
         * @type {Boolean}
         */
        $scope.isTouchDisable = !UtilFactory.isTouchEnabled();
    }

    function originHomeDiscoveryTileProgrammable(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            replace: true,
            scope: {
                imageRaw: '@image',
                descriptionRaw: '@description',
                authorname: '@authorname',
                authordescription: '@authordescription',
                authorhref: '@authorhref',
                offerId: '@offerid',
                requestedFriendType: '@requestedfriendtype',
                friendsText: '@friendstext',
                friendsPopoutTitle: '@friendspopouttitle',
                showPrice: '@showprice'
            },
            controller: 'OriginHomeDiscoveryTileProgrammableCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/discovery/tile/views/programmable.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileProgrammable
     * @restrict E
     * @element ANY
     * @scope
     * @param {ImageUrl} image the image to use
     * @param {LocalizedText} description the description text
     * @param {LocalizedString} authorname an optional author name eg. JohnDoe253
     * @param {LocalizedString} authordescription an optional author description eg. Youtube User
     * @param {Url} authorhref an optional author external link
     * @param {OfferId} offerid the offerid associatied with the programmable tile
     * @param {FriendsListEnumeration} requestedfriendtype the requested friend type if there is one
     * @param {LocalizedText} friendstext the friends text associated with the avatar list
     * @param {LocalizedText} friendspopouttitle the title shown on the friends popout title
     * @param {ShowPriceEnumeration} showprice do we want to show a price if there is a valid offer id associated with the programmable tile
     * @description
     *
     * programmable discovery tile
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-discovery-tile-programmable image="https://qa.www.assets.cms.origin.com/content/dam/originx/web/app/home/discovery/programmed/tile_sims_4_48_hr_blog.jpg" description="<strong>SIMS 4 GAME TIME</strong><br>Are you ready to rule? The Sims 4 is now available with Origin Game Time, giving players 48 hours of free, unrestricted gameplay.">
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