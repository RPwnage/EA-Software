/**
 * @file store/freegames/scripts/program-offer.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-freegames-program-offer';

    function OriginStoreFreegamesProgramOfferCtrl($scope, UtilFactory) {
        $scope.strings = {
            free: UtilFactory.getLocalizedStr($scope.freeText, CONTEXT_NAMESPACE, 'free')
        };
    }

    function originStoreProgramOffer(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                onClick: '&',
                packArt: '@',
                displayName: '@',
                originalPrice: '@',
                freeText: '@',
                description: '@',
                offerDescriptionTitle: '@',
                offerDescription: '@',
                offerHref: '@',
                offerId: '@'
            },
            controller: OriginStoreFreegamesProgramOfferCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/freegames/views/program-offer.html')
        };
    }


    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreProgramOffer
     * @restrict E
     * @element ANY
     * @scope
     * @description Used internally by freegame program and oth
     * @param {Function} on-click on click functionality
     * @param {URL} pack-art url to pack art image
     * @param {string} discount-price Discounted price of offer
     * @param {string} original-price Original price of offer
     * @param {LocalizedString} free-text "FREE" text next to strike price.
     * @param {LocalizedString} display-name Human-readable name of offer
     * @param {LocalizedString} description The text for button (Optional).
     * @param {string} offer-id The id of the offer associated with this module.
     * @param {LocalizedString} offer-description-title The description title in the offer section.
     * @param {LocalizedString} offer-description The description in the offer section.
     * @param {string} offer-href Url to the offer itself
     *
     *
     * @example
     *
     */
    angular.module('origin-components')
        .controller('OriginStoreFreegamesProgramOfferCtrl', OriginStoreFreegamesProgramOfferCtrl)
        .directive('originStoreProgramOffer', originStoreProgramOffer);
}());