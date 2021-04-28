/**
 * @file legaldisclaimer.js
 */

(function() {
    'use strict';
    var CONTEXT_NAMESPACE = 'origin-legaldisclaimer',
        UI_ROUTER_STATE_CHANGE_STATE_EVENT = 'uiRouter:stateChangeStart';

    function originLegaldisclaimer(ComponentsConfigFactory, UtilFactory, DialogFactory, AppCommFactory) {
        function originLegaldisclaimerdialogLink(scope, element){
            var legalDisclaimerLinkHtml = '<a class="otka origin-legaldisclaimer-link">' + UtilFactory.getLocalizedStr(scope.legalDisclaimerLinkText, CONTEXT_NAMESPACE, 'legal-disclaimer-link-text') + '</a>',
                legalDisclaimerTitle = '<p class="otkc origin-legaldisclaimer-title"><strong>' + scope.legalDisclaimerDialogBodyTitleText + '</strong></p>',
                legalDisclaimerBodyHtml = legalDisclaimerTitle + '<p class="otktitle-5">' + scope.legalDisclaimerDialogBodyText + '</p>';

            if (scope.legalDisclaimerText){
                scope.legalDisclaimerTextFormatted = UtilFactory.getLocalizedStr(scope.legalDisclaimerText, CONTEXT_NAMESPACE, 'legal-disclaimer-text', {
                    '%legal-disclaimer-link-text%': legalDisclaimerLinkHtml
                });
            }else{
                scope.legalDisclaimerTextFormatted = legalDisclaimerLinkHtml;
            }

            element.on('click', '.origin-legaldisclaimer-link', function(){
                DialogFactory.open({
                    id: CONTEXT_NAMESPACE,
                    size: 'large',
                    directives: [
                        {
                            id: CONTEXT_NAMESPACE,
                            name: 'origin-dialog-content-legaldisclaimer',
                            data: {
                                header: scope.legalDisclaimerDialogHeaderText,
                                body: legalDisclaimerBodyHtml
                            }
                        }
                    ]
                });
            });

            function closeModal(){
                DialogFactory.close(CONTEXT_NAMESPACE);
            }

            AppCommFactory.events.on(UI_ROUTER_STATE_CHANGE_STATE_EVENT, closeModal);

            function unbindEvent(){
                AppCommFactory.events.off(UI_ROUTER_STATE_CHANGE_STATE_EVENT, closeModal);
            }

            scope.$on('$destroy', unbindEvent);

        }
        return {
            restrict: 'E',
            scope: {
                legalDisclaimerText: '@',
                legalDisclaimerLinkText: '@',
                legalDisclaimerDialogHeaderText: '@',
                legalDisclaimerDialogBodyTitleText: '@',
                legalDisclaimerDialogBodyText: '@'
            },
            link: originLegaldisclaimerdialogLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('views/legaldisclaimer.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originLegaldisclaimer
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} legal-disclaimer-text legal claimer text
     * @param {LocalizedString} legal-disclaimer-link-text legal disclaimer link text. Will be replaced into placeholder %legal-disclaimer-link-text% in legal-disclaimer-text
     * @param {LocalizedString} legal-disclaimer-dialog-header-text legal disclaimer dialog header text.
     * @param {LocalizedString} legal-disclaimer-dialog-body-title-text legal disclaimer dialog body title
     * @param {LocalizedString} legal-disclaimer-dialog-body-text legal disclaimer dialog body text
     * @description
     *
     * legal disclaimer directive
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-legaldisclaimer>
     *         </origin-legaldisclaimer>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .directive('originLegaldisclaimer', originLegaldisclaimer);

}());
