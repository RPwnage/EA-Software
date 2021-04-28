/**
 * Factory to be used to build the infobubble mobile dialog
 * @file socialsharing.js
 */
(function() {
    'use strict';

    var DIALOG_HIDE_EVENT = 'dialog:hide',
        MOBILE_INFOBUBBLE_DIALOG_ID = 'origin-dialog-content-mobile-infobubble';

    function InfobubbleFactory(DialogFactory, AppCommFactory) {

        function getInfobubbleDialogDirectiveXML(params){
            var directiveXML = '<origin-dialog-content-mobile-infobubble dialog-id="{dialogId}" offer-id="{offerId}" action-text="{actionText}" view-details-text="{viewDetailsText}" promo-legal-id="{promoLegalId}"></origin-dialog-content-mobile-infobubble>';
            directiveXML = directiveXML.replace('{dialogId}', MOBILE_INFOBUBBLE_DIALOG_ID)
                .replace('{offerId}', params.offerId || '')
                .replace('{actionText}', params.actionText || '')
                .replace('{viewDetailsText}', params.viewDetailsText || '')
                .replace('{promoLegalId}', params.promoLegalId || '');

            return directiveXML;
        }

        function openDialog(params, callback) {
            AppCommFactory.events.once(DIALOG_HIDE_EVENT, function (response) {
                if (response.accepted && _.isFunction(callback)) {
                    callback();
                }
            });

            DialogFactory.open({
                id: MOBILE_INFOBUBBLE_DIALOG_ID,
                xmlDirective: getInfobubbleDialogDirectiveXML(params),
                size: 'medium'
            });
        }

        return {
            openDialog: openDialog
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.InfobubbleFactory
     * @description
     *
     * InfobubbleFactory
     */
    angular.module('origin-components')
        .factory('InfobubbleFactory', InfobubbleFactory);
}());
