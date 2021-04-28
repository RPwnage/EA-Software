/**
 * @file game/gamecta.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-cta-downloadinstallplay';

    function GameCTAFactory($window, LocFactory, GamesDataFactory, GamesContextFactory, UtilFactory, DialogFactory, ComponentsLogFactory, AppCommFactory) {
        function checkClientDataNotInitialized() {
            return !GamesDataFactory.initialClientStatesReceived();
        }

        // send user to ogd
        function detailsAction(offerId) {
            AppCommFactory.events.fire('uiRouter:go', 'app.game_gamelibrary.ogd', {
                offerid: offerId
            });
        }

        // display unsupported platform warning
        function incompatiablePlatform() {
            DialogFactory.openPrompt({
                id: 'store-you-need-origin-incompatible',
                title: UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'incompatibleplatformtitle'),
                description: UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'incompatibleplatformdesc'),
                rejectText: UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'continuebrowsing')
            });
        }

        function setButtonState(stateLabel, stateFunction) {
            var buttonState = {
                label: UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, stateLabel),
                clickAction: stateFunction,
                phaseDisplay: null
            };
            return buttonState;
        }

        function processPrimaryAction(primaryActionObj) {
            var buttonState;
            if (primaryActionObj.label) {
                if(primaryActionObj.label === 'getitwithoriginctaincompatible') {
                    buttonState = setButtonState('getitwithorigincta', incompatiablePlatform);
                } else {
                    buttonState = setButtonState(primaryActionObj.label, primaryActionObj.callback);
                    buttonState.disabled = !primaryActionObj.enabled;

                    if (primaryActionObj.label === 'primaryphasedisplaycta') {
                        //manually set the phase display text
                        buttonState.phaseDisplay = primaryActionObj.phaseDisplay;
                    }
                }
            } else {
                buttonState = setButtonState('detailscta', _.partial(detailsAction, primaryActionObj.offerId));
            }

            return buttonState;
        }

        function catchPrimaryActionError(error) {
            ComponentsLogFactory.error('[GAMESENTITLEMENTFACTORY:baseGameEntitlements]', error);
            return null;
        }

        function getButtonState(offerId, isMoreDetailsDisabled) {
            var buttonState = null;
            isMoreDetailsDisabled = isMoreDetailsDisabled || false;

            // The client data may take a while, in which case we need to wait
            // for the data to get back and in that time display a default.
            // If the user does not own this game, just show the primary action.
            if (checkClientDataNotInitialized() && GamesDataFactory.ownsEntitlement(offerId)) {
                buttonState = setButtonState('retrievinggamestate', null);
                return Promise.resolve(buttonState);
            } else {
                return GamesContextFactory.primaryAction(offerId, isMoreDetailsDisabled)
                    .then(processPrimaryAction)
                    .catch(catchPrimaryActionError);
            }
        }

        return {
            getButtonState: getButtonState
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function GameCTAFactorySingleton($window, LocFactory, GamesDataFactory, GamesContextFactory, UtilFactory, DialogFactory, ComponentsLogFactory, AppCommFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('GameCTAFactory', GameCTAFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GameCTAFactory

     * @description
     * {LocalizedString} incompatibleplatformtitle
     * {LocalizedString} incompatibleplatformdesc
     * {LocalizedString} continuebrowsing
     *
     * GameCTAFactory
     */
    angular.module('origin-components')
        .factory('GameCTAFactory', GameCTAFactorySingleton);
}());
