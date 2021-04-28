/**
 * @file store/oth/scripts/oth.js
 */
(function(){
    'use strict';

    function originStoreOthProgram(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                description: '@',
                freeText: '@',
                offerId: '@',
                offerDescriptionTitle: '@',
                offerDescription: '@',
                layout: '@',
                size: '@'
            },
            controller: 'OriginStoreFreegamesProgramCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/oth/views/oth.html')
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
     * @param {LocalizedString} free-text Display something other than "FREE" for the discount
     * @param {LocalizedString} description The text for button (Optional).
     * @param {string} offer-id The id of the offer associated with this module.
     * @param {LocalizedString} offer-description-title The description title in the offer section.
     * @param {LocalizedString} offer-description The description in the offer section.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-store-oth-program
     *              layout="left"
     *              description="Learn More"
     *              offer-id="OFB-EAST:109552419"
     *              offer-description-title="What kind of city will you create?"
     *              offer-description="You are an architect, cityplanner, Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Nulla vitae elit libero, a pharetra augue.">
     *          </origin-store-oth-program>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreOthProgram', originStoreOthProgram);
}());
