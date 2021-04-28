/**
 * @file store/freegames/scripts/program-offer-oth.js
 */
(function(){
    'use strict';

    function originStoreProgramOfferOth(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                onClick: '&',
                pdpUrl: '@',
                packArt: '@',
                displayName: '@',
                description: '@',
                offerDescriptionTitle: '@',
                offerDescription: '@',
                offerId: '@',
                earlyAccess: '@',
                oth: '@'
            },
            controller: 'OriginStoreFreegamesProgramOfferCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/freegames/views/program-offer-oth.html')
        };
    }


    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreProgramOfferOth
     * @restrict E
     * @element ANY
     * @scope
     * @description Used internally by freegame program and oth
     * @param {Function} on-click on click functionality
     * @param {URL} pdp-url pdp url
     * @param {URL} pack-art url to pack art image
     * @param {LocalizedString} display-name Human-readable name of offer
     * @param {LocalizedString} description The text for button (Optional).
     * @param {OfferId} offer-id The id of the offer associated with this module.
     * @param {LocalizedString} offer-description-title The description title in the offer section.
     * @param {LocalizedString} offer-description The description in the offer section.
     *
     * @example
     *
     */
    angular.module('origin-components')
        .directive('originStoreProgramOfferOth', originStoreProgramOfferOth);
}());
