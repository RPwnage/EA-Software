/**
 * @file common/urltrigger.js
 */
(function() {
    'use strict';

    var GIFTING_FEATUREINTRO_DIALOG_ID = 'origin-dialog-featureintro-gifting';

    function UrlTriggerFactory($location, AppCommFactory, DialogFactory, FeatureConfig) {
        /**
         * Launch "Gifting Feature Intro" modal
         * queryparam key: giftingintro
         */
        function showGiftingIntro() {
            if (!FeatureConfig.isGiftingEnabled()) {
                return;
            }

            DialogFactory.open({
                id: GIFTING_FEATUREINTRO_DIALOG_ID,
                size: 'large',
                directives: [{
                    name: 'origin-dialog-content-featureintro-gifting',
                    data: {
                        'dialog-id': GIFTING_FEATUREINTRO_DIALOG_ID
                    }
                }]
            });
        }

        // Functions that will get fired if respective queryString param is present in URL upon SPA start or route change
        var actionMap = {
            'giftingintro': showGiftingIntro
        };

        /**
         * Given a queryParam key, execute the respective function as defined in the actionMap
         * @param {string} key
         */
        function startAction(key) {
            actionMap[key]();

            // Remove query param from the URL once its respective action has been initiated
            $location.search(key, null);
        }

        /**
         * Search for queryParam keys that are "trigger-able" in the context of this factory and
         * execute their associated function as applicable
         */
        function startActions() {
            _.forEach(actionMap, function(actionFunction, key) {
                if (_.isFunction(actionFunction)) { // Only proceed if there is an action associated w/the key
                    var queryParamValue = _.get($location.search(), key);
                    if (queryParamValue) {
                        startAction(key);
                    }
                }
            });

            watchForRouteChanges();
        }

        /**
         * Watch for "actionable" queryString params on route changes, as they may be in merchandized links etc.
         */
        function watchForRouteChanges() {
            AppCommFactory.events.on('uiRouter:stateChangeSuccess', startActions);
        }

        return {
            startActions: startActions
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.urltrigger
     * @description
     *
     * Allows in-app functionality to be triggered by the presence of a URL query parameter (e.g. highlight certain new feature via Twitter/FB/email link)
     */
    angular.module('origin-components')
        .factory('UrlTriggerFactory', UrlTriggerFactory);

})();