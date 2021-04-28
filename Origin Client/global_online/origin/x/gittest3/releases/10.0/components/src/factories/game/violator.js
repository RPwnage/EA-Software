/**
 * @file game/violator.js
 */
(function() {
    'use strict';

    function GameViolatorFactory() {

        var RESOLVE_TIMEOUT = 6000,
            pinkySwear,
            violators;

        /**
        * Returns the list of violators in priority order with
        * the first item being the highest priority
        * @return {Array}
        * @method violatorList
        */
        function violatorList() {
            return [
                {'violator': 'billingfailed', 'directive': 'origin-game-violator-billingfailed'},
                {'violator': 'billingpending', 'directive': 'origin-game-violator-billingpending'},
                {'violator': 'preloadon', 'directive': 'origin-game-violator-preloadon'},
                {'violator': 'preloadavailable', 'directive': 'origin-game-violator-preloadavailable'},
                {'violator': 'releaseson', 'directive': 'origin-game-violator-releaseson'},
                {'violator': 'needsrepair', 'directive': 'origin-game-violator-needsrepair'},
                {'violator': 'updateavailable', 'directive': 'origin-game-violator-updateavailable'},
                {'violator': 'gameexpires', 'directive': 'origin-game-violator-gameexpires'},
                {'violator': 'trailnotactivated', 'directive': 'origin-game-violator-trialnotactivated'},
                {'violator': 'trialnotexpired', 'directive': 'origin-game-violator-trialnotexpired'},
                {'violator': 'trialexpired', 'directive': 'origin-game-violator-trialexpired'},
                {'violator': 'gamedisabled', 'directive': 'origin-game-violator-gamedisabled'},
                {'violator': 'downloadoverride', 'directive': 'origin-game-violator-downloadoverride'},
                {'violator': 'newdlcavailable', 'directive': 'origin-game-violator-newdlcavailable'},
                {'violator': 'newdlceexpansionavailable', 'directive': 'origin-game-violator-newdlcexpansionavailable'},
                {'violator': 'newdlcreadyforinstall', 'directive': 'origin-game-violator-newdlcreadyforinstall'},
                {'violator': 'newdlcinstalled', 'directive': 'origin-game-violator-newdlcinstalled'},
                {'violator': 'gamepatched', 'directive': 'origin-game-violator-gamepatched'},
                {'violator': 'gametobesunset', 'directive': 'origin-game-violator-gametobesunset'},
                {'violator': 'gamesunsetted', 'directive': 'origin-game-violator-gamesunsetted'}
            ];
        }

        /**
        * Lets us know of the offerId has a violator to display
        * @param {String} offerId
        * @return {Boolean}
        * @method hasViolator
        */
        function hasViolator() {
            return Math.random() > 0.5;
        }

        /**
        * Return a random directive name to display for the content
        * @return {String}
        * @method getViolatorContent
        */
        function getViolatorContent() {
            return violators[Math.floor(Math.random() * violators.length)].directive;
        }

        /**
        * Gets the data that it needs from OCD and such
        * to determine if the game has a violator
        * @return {Promise}
        * @method init
        */
        function init() {
            if (!pinkySwear) {
                pinkySwear = new Promise(function(resolve) {
                    setTimeout(function() {
                        violators = violatorList();
                        resolve({});
                    }, RESOLVE_TIMEOUT);
                });
            }
            return pinkySwear;
        }

        return {
            getViolatorContent: getViolatorContent,
            hasViolator: hasViolator,
            init: init
        };
    }

    /**
    * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
    * @param {object} SingletonRegistryFactory the factory which manages all the other factories
    */
    function GameViolatorFactorySingleton(DialogFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('GameViolatorFactory', GameViolatorFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GameViolatorFactory

     * @description
     *
     * GameViolatorFactory
     */
    angular.module('origin-components')
        .factory('GameViolatorFactory', GameViolatorFactorySingleton);
}());