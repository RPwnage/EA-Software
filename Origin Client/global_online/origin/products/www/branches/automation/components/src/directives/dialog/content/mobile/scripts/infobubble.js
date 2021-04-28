/**
 * @file dialog/content/mobile/scripts/infobubble.js
 */
(function(){
    'use strict';

    function originDialogContentMobileInfobubbleCtrl($scope, ProductFactory, PdpUrlFactory, DialogFactory) {

        function modelReady(model){
            $scope.model = model;
            $scope.gotoPdp = function($event){
                $event.preventDefault();
                DialogFactory.setReturnValues(false);
                $scope.closeModal();
                PdpUrlFactory.goToPdp($scope.model);
            };
            $scope.onActionButtonClick = function(){
                DialogFactory.setReturnValues(true);
                $scope.closeModal();
            };
            $scope.getPdpUrl = function(){
                return PdpUrlFactory.getPdpUrl($scope.model);
            };
        }

        $scope.closeModal = function (){
            DialogFactory.close($scope.dialogId);
        };

        ProductFactory.get($scope.offerId).attachToScope($scope, modelReady);
    }

    function originDialogContentMobileInfobubble(ComponentsConfigFactory) {

        function originDialogContentMobileInfobubbleLink(scope, element) {
            element.on('click', '.origin-storegamelegal a', function (){
                scope.closeModal();
            });
        }

        return {
            restrict: 'E',
            scope: {
                viewDetailsText: '@',
                offerId: '@',
                actionText: '@',
                dialogId: '@',
                promoLegalId: '@'
            },
            controller: originDialogContentMobileInfobubbleCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/mobile/views/infobubble.html'),
            link: originDialogContentMobileInfobubbleLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentMobileInfobubble
     * @restrict E
     * @element ANY
     * @scope
     *
     * @param {LocalizedString} view-details-text * Button to take user to PDP page (Merchadise in defaults)
     * @param {string} offer-id * offer id (Not merchandised)
     * @param {LocalizedString} action-text * Text for the button that does the desired action ie 'Buy' or 'Add to Cart' (Not merchandised)
     * @param {String} dialog-id * Id of the dialog (Not merchandised)
     * @param {String} action-event * Name of the event to fire when the user wants to confirm the action (Not merchandised)
     * @param {String} promo-legal-id * Legal Id of the promotion that links user to Terms and Conditions page of that promotion (Not merchandised)
     *
     * @description
     *
     * Modal for implementing the info bubble flow on mobile.
     * For instance, the user clicks on direct action button on a mobile device (buy or add to cart) on a non PDP page
     * and we need to show legal items before proceeding.
     *
     * @example
     * <origin-dialog-content-mobile-infobubble></origin-dialog-content-mobile-infobubble>
     */
    angular.module('origin-components')
        .directive('originDialogContentMobileInfobubble', originDialogContentMobileInfobubble);
}());
