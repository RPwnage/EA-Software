/**
 * @file game/gamecta.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-cta-downloadinstallplay';

    function GameCTAFactory($window, LocFactory, GamesDataFactory, GamesContextFactory, UtilFactory, DialogFactory, ComponentsLogFactory) {

        function checkClientDataNotInitialized() {
            return !GamesDataFactory.initialClientStatesReceived();
        }

        function learnMoreAction() {
            //TEMP
            DialogFactory.openAlert({
                id: 'web-going-to-pdp-3',
                title: 'Going to PDP',
                description: 'When the rest of OriginX is fully functional, you will be taken to the games owned details page.',
                rejectText: 'OK'
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
                buttonState = setButtonState(primaryActionObj.label, primaryActionObj.callback);
                buttonState.disabled = !primaryActionObj.enabled;

                if (primaryActionObj.label === 'primaryphasedisplaycta') {
                    //manually set the phase display text
                    buttonState.phaseDisplay = primaryActionObj.phaseDisplay;
                }
            } else {
                buttonState = setButtonState('learnmorecta', learnMoreAction);
            }
            return buttonState;
        }

        function catchPrimaryActionError(error) {
            ComponentsLogFactory.error('[GAMESENTITLEMENTFACTORY:baseGameEntitlements]', error.stack);
            return null;
        }

        function getButtonState(offerId) {
            var buttonState = null,
                promise = null;

            if (checkClientDataNotInitialized()) {
                buttonState = setButtonState('retrievinggamestate', null);
                promise = Promise.resolve(buttonState);
            } else {
                promise = GamesContextFactory.primaryAction(offerId)
                    .then(processPrimaryAction)
                    .catch(catchPrimaryActionError);
            }
            return promise;
        }

        return {
            getButtonState: getButtonState
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function GameCTAFactorySingleton($window, LocFactory, GamesDataFactory, GamesContextFactory, UtilFactory, DialogFactory, ComponentsLogFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('GameCTAFactory', GameCTAFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GameCTAFactory

     * @description
     *
     * GameCTAFactory
     */
    angular.module('origin-components')
        .factory('GameCTAFactory', GameCTAFactorySingleton);
}());