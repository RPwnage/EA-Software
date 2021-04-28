/**
 * @file store/freegames/scripts/program.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-freegames-program';

    function OriginStoreFreegamesProgramCtrl($scope, UtilFactory, ProductFactory, DialogFactory, PdpUrlFactory) {
        $scope.description = UtilFactory.getLocalizedStr('', CONTEXT_NAMESPACE, 'offerGetItNow');

        $scope.goToPdp = PdpUrlFactory.goToPdp;

        $scope.model = {};
        $scope.$watch('offerId', function (newValue) {
            if (newValue) {
                ProductFactory.get($scope.offerId).attachToScope($scope, 'model');
            }
        });

        $scope.openModal = function(num) {
            DialogFactory.openDirective({
                id: 'web-youtube-video',
                name: 'origin-dialog-content-media-carousel',
                size: 'large',
                data: {
                    items: $scope.model.media,
                    active: num
                }
            });
        };
    }

    function originStoreFreegamesProgram(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                description: '@',
                offerId: '@',
                size: '@',
                offerDescriptionTitle: '@',
                offerDescription: '@',
                layout: '@'
            },
            controller: 'OriginStoreFreegamesProgramCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/freegames/views/program.html')
        };
    }

    /* jshint ignore:start */
    /**
     * Sets the layout of this module.
     * @readonly
     * @enum {string}
     */
    var LayoutEnumeration = {
        "Left": "left",
        "Right": "right"
    };

    /* jshint ignore:end */

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreFreegamesProgram
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LayoutEnumeration} layout The position of the game card compared to the screenshots.
     * @param {LocalizedString} description The text for button (Optional).
     * @param {string} offer-id The id of the offer associated with this module.
     * @param {ImageUrl} image The background image for this module.
     * @param {LocalizedString} offer-description-title The description title in the offer section.
     * @param {LocalizedString} offer-description The description in the offer section.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-store-freegames-program
     *              layout="left"
     *              description="Learn More"
     *              offer-id="OFB-EAST:109552419"
     *              image="http://docs.x.origin.com/origin-x-design-prototype/images/backgrounds/ultima.jpg"
     *              offer-description-title="What kind of city will you create?"
     *              offer-description="You are an architect, cityplanner, Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Nulla vitae elit libero, a pharetra augue.">
     *          </origin-store-freegames-program>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreFreegamesProgramCtrl', OriginStoreFreegamesProgramCtrl)
        .directive('originStoreFreegamesProgram', originStoreFreegamesProgram);
}());
