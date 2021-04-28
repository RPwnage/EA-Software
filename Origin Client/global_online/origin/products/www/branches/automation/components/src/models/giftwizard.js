/**
 *  * @file models/giftwizard.js
 */
(function () {
    'use strict';

    var FRIEND_LIST_OVERLAY_ID = 'origin-gift-friendslist-overlay';
    var CUSTOM_MESSAGE_OVERLAY_ID = 'origin-gift-custommessage-overlay';

    function GiftingWizardFactory(DialogFactory, AuthFactory, GiftingFactory, PurchaseFactory, StoreAgeGateFactory, GiftFriendListFactory, ErrorModalFactory) {
        var giftingDialogOverCounter = 0,
            friendListOverlayDialogId = FRIEND_LIST_OVERLAY_ID;
        /**
         * Launches friend list overlay
         * @param offerId
         */
        function startFriendListGiftingFlow(offerId) {
            //CPP dialog is closing the same dialog again on an event but the event is not triggered synchronously, which causes the dialog to close if opened again
            //So we alternate the dialog ID with a suffix of 0 or 1
            giftingDialogOverCounter = ++giftingDialogOverCounter % 2;
            friendListOverlayDialogId = CUSTOM_MESSAGE_OVERLAY_ID + giftingDialogOverCounter;
            DialogFactory.close(CUSTOM_MESSAGE_OVERLAY_ID);
            DialogFactory.openDirective({
                id: friendListOverlayDialogId,
                name: 'origin-store-gift-friendslist',
                size: 'large',
                data: {
                    id: friendListOverlayDialogId,
                    offerid: offerId
                }
            });
            Origin.telemetry.events.fire(Origin.telemetry.events.TELEMETRY_GIFTING_START_FLOW);
        }

        /**
         * Launches customize message overlay
         * @param {string} offerId
         * @param {Object} userData
         * @param hideBackButton
         */
        function launchCustomizeMessageDialog(offerId, userData, hideBackButton) {

            DialogFactory.close(friendListOverlayDialogId);
            DialogFactory.openDirective({
                id: CUSTOM_MESSAGE_OVERLAY_ID,
                name: 'origin-store-gift-custommessage',
                size: 'large',
                data: {
                    id: CUSTOM_MESSAGE_OVERLAY_ID,
                    offerid: offerId,
                    nucleusid: userData.nucleusId,
                    hidebackbtn: hideBackButton
                }
            });
            Origin.telemetry.events.fire(Origin.telemetry.events.TELEMETRY_GIFTING_MESSAGE_PAGE);
        }

        /**
         *
         * @param {string} offerId
         * @param {Object} userData
         * @param {boolean} hideBackButton
         */
        function startCustomizeMessageFlow(offerId, userData, hideBackButton) {
            launchCustomizeMessageDialog(offerId, userData, hideBackButton);
        }

        /**
         * Calls startCustomizeMessageFlow
         * @param {Object} game
         * @param {Object} userData
         */
        function loginAndStartCustomizeMessageFlow(game, userData) {
            if (GiftingFactory.isGiftingEnabled()) {
                AuthFactory.promiseLogin()
                    .then(_.partial(StoreAgeGateFactory.validateUserAgePromise, game))
                    .then(_.partial(startCustomizeMessageFlow, game.offerId, userData, true))
                    .catch(ErrorModalFactory.showDialog);
            }
        }

        /**
         * Calls startCustomizeMessageFlow, but hides back button on customize message overlay
         * @param {string} offerId
         * @param {Object} userData
         */
        function continueToCustomizeMessageFlow(offerId, userData) {
            startCustomizeMessageFlow(offerId, userData, false);
        } 

        /**
         * Closes customize message overlay and launches checkout
         * @param offerId
         * @param nucleusId
         * @param recipientName
         * @param message
         */
        function continueToCheckout(offerId, nucleusId, recipientName, message) {
            DialogFactory.close(CUSTOM_MESSAGE_OVERLAY_ID);
            PurchaseFactory.gift({
                offerId: offerId,
                recipientId: nucleusId,
                senderName: recipientName,
                message: message
            });
        }

        /**
         * Triggers login flow if necessary then launches friend list flow.
         * @param {Object} game
         */
        function loginAndStartGiftingFlow(game) {
            if (GiftingFactory.isGiftingEnabled()) {
                AuthFactory.promiseLogin()
                    .then(_.partial(StoreAgeGateFactory.validateUserAgePromise, game))
                    .then(_.partial(GiftFriendListFactory.reset))
                    .then(_.partial(startFriendListGiftingFlow, game.offerId))
                    .catch(ErrorModalFactory.showDialog);
            }
        }

        /**
         * Context menu handler for gifting item
         * @param {Object} contextMenu
         */
        function startGiftingFlowContextMenuHandler(contextMenu) {
            loginAndStartGiftingFlow(_.get(contextMenu, ['game']));
        }

        /**
         * creates a gifting context menu item
         * @param game
         * @returns {Object} context menu item properties.
         */
        function getGiftContextMenu(game) {
            if (GiftingFactory.isGiftingEnabled()) {
                return {
                    label: 'gift',
                    callback: startGiftingFlowContextMenuHandler,
                    enabled: true,
                    divider: false,
                    game: game,
                    tealiumType: 'Gift'
                };
            }
        }

        return {
            loginAndStartGiftingFlow: loginAndStartGiftingFlow,
            getGiftContextMenu: getGiftContextMenu,
            continueToCustomizeMessageFlow: continueToCustomizeMessageFlow,
            continueToCheckout: continueToCheckout,
            startFriendListGiftingFlow: startFriendListGiftingFlow,
            loginAndStartCustomizeMessageFlow: loginAndStartCustomizeMessageFlow
        };
    }


    /**
     * @ngdoc service
     * @name origin-components.factories.GiftingWizardFactory
     * @description
     *
     * GiftingWizardFactory
     */
    angular.module('origin-components')
        .factory('GiftingWizardFactory', GiftingWizardFactory);
}());
