/**
 * @file store/freegames/scripts/offermodule.js
 */
(function () {
    'use strict';

    /* jshint ignore:start */
    /**
     * Sets the layout of this module.
     * @readonly
     * @enum {string}
     */
    var AlignmentEnumeration = {
        "Left": "left",
        "Right": "right"
    };
    /* jshint ignore:end */

    var CONTEXT_NAMESPACE = 'origin-store-freegames-offermodule';


    function originStoreFreegamesOffermodule(ComponentsConfigFactory, DirectiveScope) {

        function originStoreFreegamesOffermoduleLink(scope) {
            DirectiveScope.populateScopeWithOcdAndCatalog(scope, CONTEXT_NAMESPACE, scope.ocdPath);
        }

        return {
            restrict: 'E',
            scope: {
                alignment: '@',
                titleStr: '@',
                subtitle: '@',
                href: '@',
                description: '@',
                ocdPath: '@',
                offersubtitle: '@',
                offerdescription: '@',
                startColor: '@',
                endColor: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/freegames/views/offermodule.html'),
            link: originStoreFreegamesOffermoduleLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreFreegamesOffermodule
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {AlignmentEnumeration} alignment The text alignment of this module.
     * @param {LocalizedString} title-str The title for this module.
     * @param {LocalizedString} subtitle The title for this module.
     * @param {string} href The URL for button below the text.
     * @param {LocalizedString} description The text for button below the text.
     * @param {OcdPath} ocd-path OCD Path associated with this module.
     * @param {LocalizedString} offersubtitle The subtitle in the offer section.
     * @param {LocalizedString} offerdescriptiontitle The title of description in the offer section.
     * @param {LocalizedString} offerdescription The description in the offer section.
     * @param {string} start-color The start of the fade color for the background
     * @param {string} end-color The end color/background color of the module
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-freegames-offermodule
     *          alignment="left"
     *          title-str="Demos & Betas"
     *          subtitle="Experience a game before it’s released, or get a sample of the final game. Betas and demos are a great way to see what the games are all about."
     *          href="free-games/demos-betas"
     *          description="Learn More"
     *          ocd-path="/simcity/simcity/trial"
     *          offersubtitle="Play for 48 free hours."
     *          offerdescriptiontitle="Download the FIFA 15 Demo"
     *          offerdescription="Feel the game. Try the Ignite Engine for free.">
     *      </origin-store-freegames-offermodule>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreFreegamesOffermodule', originStoreFreegamesOffermodule);
}());
