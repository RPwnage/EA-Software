
/**
 * @file store/access/scripts/takeover.js
 */
(function(){
    'use strict';

    var NAMESPACE = 'origin-store-access-takeover';

    /* jshint ignore:start */
    var ColorEnumeration = {
        "light": "light",
        "dark": "dark"
    };

    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    function OriginStoreAccessTakeoverCtrl($scope, PriceInsertionFactory) {
        $scope.strings = {};
        $scope.btnColor = $scope.color === 'dark' ? 'dark' : 'transparent';

        PriceInsertionFactory
            .insertPriceIntoLocalizedStr($scope, $scope.strings, $scope.header, NAMESPACE, 'header');
    }

    function originStoreAccessTakeover(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            transclude: true,
            scope: {
                header: '@',
                image: '@',
                primaryCtaText: '@',
                secondaryCtaText: '@',
                backgroundColor: '@',
                videoId: '@',
                matureContent: '@',
                offerId: '@',
                color: '@',
                ctid: '@'
            },
            controller: 'OriginStoreAccessTakeoverCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/access/views/takeover.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAccessTakeover
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedTemplateString} header The main header
     * @param {ImageUrl} image The background image
     * @param {LocalizedString} primary-cta-text The primary CTA button text
     * @param {LocalizedString} secondary-cta-text The secondary CTA button text
     * @param {String} background-color The background color of the page
     * @param {Video} video-id YouTube video ID
     * @param {BooleanEnumeration} mature-content Whether the video(s) defined within this component should be considered Mature
     * @param {String} offer-id The subscription offer id to purchase in the subs flow
     * @param {ColorEnumeration} color Font color
     * @param {string} ctid Campaign Targeting ID
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-access-takeover
     *          header="some header"
     *          image="http://someimage.jpg"
     *          primary-cta-text="Join Now"
     *          secondary-cta-text="Learn More"
     *          video-id="sadf76a"
     *          mature-content="true"
     *          color="light"
     *          offer-id="OR:12345.505">
     *     <origin-store-access-takeover>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreAccessTakeoverCtrl', OriginStoreAccessTakeoverCtrl)
        .directive('originStoreAccessTakeover', originStoreAccessTakeover);
}());
