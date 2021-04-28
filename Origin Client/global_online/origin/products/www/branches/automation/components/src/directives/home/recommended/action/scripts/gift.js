/**
 * @file home/recommended/gift.js
 */
(function() {
    'use strict';
    var CONTEXT_NAMESPACE = 'origin-home-recommended-action-gift';

    function OriginHomeRecommendedActionGiftCtrl($scope, AppCommFactory, ComponentsLogFactory, UtilFactory, GamesDataFactory, GiftingFactory, GiftStateConstants) {

        function tryOpenGift() {
            sendTelemetry();
            AppCommFactory.events.fire('uiRouter:go', 'app.gift_unveiling', {
                offerId: $scope.offerId,
                fromSPA: true
            });
        }

        $scope.onBtnClick = function(e) {
            e.stopPropagation();
            tryOpenGift();
        };

        /**
         * logs an error if we fail to retrieve the catalog
         * @param  {Error} error the error object
         */
        function handleError(error) {
            ComponentsLogFactory.error('[OriginHomeRecommendedActionGiftCtrl]', error);
        }

        /**
         * sets the bindings in the template
         * @param {string} offerId offer id
         */
        function setBindings(offerId) {

            $scope.offerId = offerId || $scope.offerId;

            //grab the image taking into account CQ Defaults
            $scope.discoverTileGiftImage = UtilFactory.getLocalizedStr($scope.discoverTileGiftImage, CONTEXT_NAMESPACE, 'discover-tile-gift-image');

            //if we grab a string back that doesn't start with http (i.e. missdiscover-tile-gift-image) don't treat that as a valid URL
            if(!_.startsWith($scope.discoverTileGiftImage, 'http')) {
                $scope.discoverTileGiftImage = null;
            }

            $scope.image = $scope.discoverTileGiftImage;
            $scope.backupImage = !$scope.discoverTileGiftImage;

            $scope.ctaText = UtilFactory.getLocalizedStr($scope.ctaText, CONTEXT_NAMESPACE, 'ctatext');

            var gift = GiftingFactory.getGift($scope.offerId, GiftStateConstants.RECEIVED);

            var sender = _.get(gift, ['senderName']);
            $scope.subtitle = UtilFactory.getLocalizedStr($scope.subtitleRaw, CONTEXT_NAMESPACE, 'subtitle', {'%sender%': sender});
            $scope.description = UtilFactory.getLocalizedStr($scope.descriptionRaw, CONTEXT_NAMESPACE, 'description');

            $scope.$digest();
        }

        $scope.touched = function() {
            tryOpenGift();
        };

        function nonDirectiveSupportTextVisible() {
            return ($scope.subtitle && $scope.subtitle.length > 0) ||
                ($scope.description && $scope.description.length > 0);
        }

        function updateSupportTextVisible() {
            $scope.supportTextVisible = nonDirectiveSupportTextVisible();
            $scope.$digest();
        }

        function setSupportTextVisible() {
             $scope.supportTextVisible = false;
        }

        function sendTelemetry(format) {
            var source = Origin.client.isEmbeddedBrowser() ? 'client' : 'web';
            format = format? format: 'live_tile';

            Origin.telemetry.sendStandardPinEvent(Origin.telemetry.trackerTypes.TRACKER_MARKETING, 'message', source, 'success', {
                'type': 'in-game',
                'service': 'originx',
                'format': format,
                'client_state': 'foreground',
                'msg_id': $scope.ctid? $scope.ctid: $scope.image,
                'status': 'click',
                'content': $scope.description,
                'destination_name': 'gift'
            });
        }

        /**
         * Is touch disabled on this device
         * @type {Boolean}
         */
        $scope.isTouchDisable = !UtilFactory.isTouchEnabled();

        setSupportTextVisible();

        GamesDataFactory.lookUpOfferIdIfNeeded($scope.ocdPath, $scope.offerId)
            .then(setBindings)
            .then(updateSupportTextVisible)
            .catch(handleError);

    }

    function originHomeRecommendedActionGift(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                subtitleRaw: '@subtitle',
                descriptionRaw: '@description',
                offerId: '@offerid',
                ocdPath: '@',
                discoverTileGiftImage: '@',
                discoverTileColor: '@',
                ctaText: '@ctatext'
            },
            controller: 'OriginHomeRecommendedActionGiftCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/recommended/action/views/gift.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeRecommendedActionGift
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} subtitle the subtitle for the tile - use %sender% to place sender's name in the string
     * @param {LocalizedString} description the description for the tile
     * @param {LocalizedString} cantopentitle the title for the 'Cannot open gift at the moment' error dialog
     * @param {LocalizedString} cantopendescription the description for the 'Cannot open gift at the moment' error dialog
     * @param {LocalizedString} closestr close button string for the 'Cannot open gift at the moment' error dialog
     * @param {string} offerid the offerid of the game being gifted
     * @param {OcdPath} ocd-path the ocd path of the game being gifted
     * @param {ImageUrl} discover-tile-gift-image gift tile image 1000x250
     * @param {string} discover-tile-color the background color
     * @param {LocalizedString} ctatext cta text for the button
     * @description
     *
     * just acquired recommended next action
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-recommended-action-gift
     *             subtitle="You have an unopened gift from %sender%"
     *             description="What are you waiting for? Open it up and see what\'s inside."
     *             cantopentitle="Cannot Open Gift"
     *             cantopendescription="Sorry, you cannot open your gift at the moment. Try again later."
     *             closestr="OK"
     *             offerid="OFB-EAST:abc123"
     *             ocd-path="path/to/ocd/content"
     *             discover-tile-gift-image="http://www.example.com/gift-tile.jpg"
     *             discover-tile-color="#FF00FF"
     *             ctatext="Open Gift"
     *         ></origin-home-recommended-action-gift>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginHomeRecommendedActionGiftCtrl', OriginHomeRecommendedActionGiftCtrl)
        .directive('originHomeRecommendedActionGift', originHomeRecommendedActionGift);
}());
