/**
 * @file cta/directacquistion.js
 */

(function() {
    'use strict';

    var CLS_ICON_LEARNMORE = 'otkicon-learn-more';

    /**
     * A list of button type options
     * @readonly
     * @enum {string}
     */
    var ButtonTypeEnumeration = {
        "primary": "primary",
        "secondary": "secondary",
        "transparent": "transparent"
    };


    function OriginCtaDirectacquisitionCtrl($scope, DialogFactory, AuthFactory) {
        $scope.iconClass = CLS_ICON_LEARNMORE;
        $scope.type = $scope.type || ButtonTypeEnumeration.primary;

        $scope.onBtnClick = function($event) {
            $event.stopPropagation();

            AuthFactory.events.fire('auth:loginWithCallback', function(){
                // TODO:  only show modal after acquisition has been confirmed - for now, we're assuming success
                // and explicitly passing along the offer ID
                if ($scope.offerId) {
                    DialogFactory.openDirective({
                        contentDirective: 'origin-dialog-content-checkoutconfirmation',
                        id: 'checkout-confirmation',
                        name: 'origin-dialog-content-checkoutconfirmation',
                        size: 'large',
                        data: {
                            offers: [$scope.offerId]
                        }
                    });
                }
            });
        };

        /* this is used to stop propagation, if this lives inside of a clickable area */
        $scope.onInfobubbleClick = function($event) {
            $event.stopPropagation();
        };
    }

    function originCtaDirectacquisition(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                link: '@href',
                btnText: '@description',
                type: '@type',
                showInfoBubble: '@showinfobubble',
                offerId: '@'
            },
            controller: 'OriginCtaDirectacquisitionCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('cta/views/cta.html')
        };
    }

    /**
     * Show or Hide the info bubble
     * @readonly
     * @enum {string}
     */
    /* jshint ignore:start */
    var showInfoBubbleEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */
    /**
     * @ngdoc directive
     * @name origin-components.directives:originCtaDirectacquisition
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} description the button text
     * @param {ButtonTypeEnumeration} type can be one of two values 'primary'/'secondary'. this sets the style of the button
     * @param {Url} href the internal origin link that will entitle a user and switch them to the game library
     * @param {showInfoBubbleEnumeration} showinfobubble - If info bubble with ratings is shown.
     * @param {string} offerid the offer id for the CTA
     *
     * @description
     * Direct Acquistion call to action button
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-cta-directacquisition
     *             href="http://internalentitlelink"
     *             description="Entitle Me To Free Game"
     *             type="primary"
     *             showinfobubble="true"
     *             offer-id="OFFER-123">
     *         </origin-cta-directacquisition>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginCtaDirectacquisitionCtrl', OriginCtaDirectacquisitionCtrl)
        .directive('originCtaDirectacquisition', originCtaDirectacquisition);
}());