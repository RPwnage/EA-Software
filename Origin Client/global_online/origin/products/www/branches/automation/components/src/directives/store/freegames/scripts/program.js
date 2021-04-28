/**
 * @file store/freegames/scripts/program.js
 */
(function(){
    'use strict';


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

    var WidthEnumeration = {
        "full": "full",
        "half": "half",
        "third": "third",
        "quarter": "quarter"
    };

    /* jshint ignore:end */

    var CONTEXT_NAMESPACE = 'origin-store-freegames-program';

    function OriginStoreFreegamesProgramCtrl($scope, UtilFactory, DialogFactory) {
        $scope.description = UtilFactory.getLocalizedStr($scope.description, CONTEXT_NAMESPACE, 'description');

        $scope.openModal = function(num) {
            DialogFactory.openDirective({
                id: 'web-youtube-video',
                name: 'origin-dialog-content-media-carousel',
                size: 'xLarge',
                data: {
                    items: $scope.screenshots,
                    active: num
                }
            });
        };
    }

    function originStoreFreegamesProgram(ComponentsConfigFactory, DirectiveScope, PdpUrlFactory) {

        function originStoreFreegamesProgramLink(scope) {
            var promise;
            function wrapInObject(imageUrl){
                return {
                    src: imageUrl,
                    type: 'image'
                };
            }
            function prepareScreenshots(){
                var screenshots = _.compact([scope.screenshot1, scope.screenshot2, scope.screenshot3, scope.screenshot4]);
                if (screenshots.length > 0){
                    scope.screenshots = _.map(screenshots, wrapInObject);
                }
            }

            promise = DirectiveScope.populateScopeWithOcdAndCatalog(scope, CONTEXT_NAMESPACE, scope.ocdPath).then(prepareScreenshots);
            scope.goToPdp = function($event) {
                $event.preventDefault();
                promise.then(function(){
                    PdpUrlFactory.goToPdp(scope.model);
                });
            };

            scope.getPdpUrl = PdpUrlFactory.getPdpUrl;
        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
                description: '@',
                ocdPath: '@',
                size: '@',
                offerDescriptionTitle: '@',
                offerDescription: '@',
                layout: '@',
                screenshot1: '@',
                screenshot2: '@',
                screenshot3: '@',
                screenshot4: '@'
            },
            link: originStoreFreegamesProgramLink,
            controller: 'OriginStoreFreegamesProgramCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/freegames/views/program.html')
        };
    }


    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreFreegamesProgram
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LayoutEnumeration} layout The position of the game card compared to the screenshots.
     * @param {LocalizedString} description The text for button (Optional).
     * @param {OcdPath} ocd-path OCD Path associated with this module.
     * @param {WidthEnumeration} size width of box
     * @param {LocalizedString} offer-description-title The description title in the offer section.
     * @param {LocalizedString} offer-description The description in the offer section.
     * @param {ImageUrl} screenshot1 screenshot 1 for the screenshot tiles
     * @param {ImageUrl} screenshot2 screenshot 2 for the screenshot tiles
     * @param {ImageUrl} screenshot3 screenshot 3 for the screenshot tiles
     * @param {ImageUrl} screenshot4 screenshot 4 for the screenshot tiles
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-store-freegames-program
     *              layout="left"
     *              description="Learn More"
     *              ocd-path="/koa/kingdoms-of-amalur-reckoning/trial"
     *              image="https://docs.x.origin.com/proto/images/backgrounds/ultima.jpg"
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
