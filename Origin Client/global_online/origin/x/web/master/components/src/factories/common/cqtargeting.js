/**
 * @file cache.js
 */
(function() {
    'use strict';

    function CQTargetingFactory(ObjectHelperFactory) {

        /**
         * check the subscription state
         * @param  {string} attributeValue the targeting attribute value
         * @return {boolean}               true if there's a match
         */
        function isSubscriptionStateTargetMatch(attributeValue) {
            var subscriber = attributeValue.toLowerCase() === 'true';
            return subscriber === SubscriptionFactory.userHasSubscription();
        }

        /**
         * check the underage state
         * @param  {string} attributeValue the targeting attribute value
         * @return {boolean}               true if there's a match
         */
        function isUnderageStateTargetMatch(attributeValue) {
            var underaged = attributeValue.toLowerCase() === 'true';
            return underaged === Origin.user.underAge();
        }

        /**
         * check the offerid for ownership
         * @param  {string} attributeValue the targeting attribute value
         * @return {boolean}               true if there's a match
         */
        function isOfferIdTargetMatch(attributeValue) {
            return !!GamesDataFactory.getEntitlement(attributeValue);

        }

        /**
         * check the mastertitleid for ownership
         * @param  {string} attributeValue the targeting attribute value
         * @return {boolean}               true if there's a match
         */
        function isMasterTitleIdTargetMatch(attributeValue) {
            return !!GamesDataFactory.getBaseGameEntitlementByMasterTitleId(attributeValue);
        }

        /**
         * "or" two values and returns the result
         * @param  {boolean} value1 boolean param 1
         * @param  {boolean} value2 boolean param 2
         * @return {boolean}        the result of the "or"
         */
        function or(value1, value2) {
            return value1 || value2;
        }

        /**
         * "ands" two values and returns the result
         * @param  {boolean} value1 boolean param 1
         * @param  {boolean} value2 boolean param 2
         * @return {boolean}        the result of the "and"
         */
        function and(value1, value2) {
            return value1 && value2;
        }

        /**
         * takes the inverse of a boolean value
         * @param  {boolean} value the value to not
         * @return {function} returns a function that takes in attribute value, runs the check function and returns the not
         */
        function not(attributeCheckFunction) {
            return function(attributeValue) {
                return !attributeCheckFunction(attributeValue);
            };
        }

        /**
         * for a targeting attribute that has a comma deliminated string as a value
         * it will run the targeting check on each value and run a logic operator between each value
         * @param  {function}  attributeCheckFunction the attribute check function
         * @param  {function}  logicFunction          the "and" or "or" function to be passed in
         * @param  {boolean}  not                    set true if you want to take the inverse of the result
         * @return {function}                        function that takes in a comma deliminated string and returns a bool
         */
        function isMultipleValueMatch(attributeCheckFunction, logicFunction, not) {
            return function(attributeValue) {
                var returnValue = false,
                    valuesArray = attributeValue.split(',');

                for (var i = 0; i < valuesArray.length; i++) {
                    returnValue = logicFunction(returnValue, attributeCheckFunction(valuesArray[i]));
                }
                return not ? (!returnValue) : returnValue;
            };
        }

        /**
         * check for a match for one of the targeting attributes
         * @param  {string} attributeName the name of the targeting attribute we are checking
         * @param  {string} attributeValue the value of the targeting attribute
         * @return {boolean}                       true/false depending if the attribute matches the criteria
         */
        function targetingAttributeMatches(attributeName, attributeValue) {
            var attributeMatchCheckFunction = targetingConfigs[attributeName];
            return attributeMatchCheckFunction ? attributeMatchCheckFunction(attributeValue) : true;

        }

        /**
         * wait for the CQTargeting dependent sources to be ready
         * @return {Promise} resolves when the data source is ready
         */
        function waitForDataSourcesReady() {
            return new Promise(function(resolve) {

                if (GamesDataFactory.initialRefreshCompleted()) {
                    resolve();
                } else {
                    GamesDataFactory.events.once('games:baseGameEntitlementsUpdate', resolve);
                    GamesDataFactory.events.once('games:baseGameEntitlementsError', resolve);
                }
            });

        }

        /**
         * takes targeting information and returns true if all the targeting criteria is met, false if it is not.
         * if the targetingObject is undefined or null, we assume no target information and the function will return true.
         * @param  {object} targetingObject the object containing targeting information
         * @return {boolean}                   true if we match all the criteria, false otherwise
         */


        function targetingItemMatches(targetingObject) {
            var matched = true;

            //if we don't have a valid targeting object we just return it as a match
            if (targetingObject) {
                ObjectHelperFactory.forEach(function(attributeValue, attributeName) {
                    if (angular.isDefined(attributeValue) && (attributeValue !== null)) {
                        matched = matched && !!targetingAttributeMatches(attributeName, attributeValue);
                    }
                }, targetingObject);
            }

            return matched;
        }

        /**
         * init the factory with some dependencies
         * @param  {GamesDataFactory} incomingGamesDataFactory the GamesDataFactory
         * @param  {SubscriptionFactory} incomingSubsFactory   the SubscriptionDataFactory
         */
        function init(incomingGamesDataFactory, incomingSubsFactory) {
            //we don't use angular injection here to avoid circular dependency
            GamesDataFactory = incomingGamesDataFactory;
            SubscriptionFactory = incomingSubsFactory;
        }

        //the targeting attributes we look for
        var targetingConfigs = {
            'cq-targeting-subscribers': isSubscriptionStateTargetMatch,
            'cq-targeting-underage': isUnderageStateTargetMatch,
            'cq-targeting-offerid': isOfferIdTargetMatch,
            'cq-targeting-offerid-and': isMultipleValueMatch(isOfferIdTargetMatch, and),
            'cq-targeting-offerid-or': isMultipleValueMatch(isOfferIdTargetMatch, or),
            'cq-targeting-offerid-nand': isMultipleValueMatch(isOfferIdTargetMatch, and, true),
            'cq-targeting-offerid-nor': isMultipleValueMatch(isOfferIdTargetMatch, or, true),
            'cq-targeting-offerid-not': not(isOfferIdTargetMatch),
            'cq-targeting-mastertitleid': isMasterTitleIdTargetMatch,
            'cq-targeting-mastertitleid-and': isMultipleValueMatch(isMasterTitleIdTargetMatch, and),
            'cq-targeting-mastertitleid-or': isMultipleValueMatch(isMasterTitleIdTargetMatch, or),
            'cq-targeting-mastertitleid-nand': isMultipleValueMatch(isMasterTitleIdTargetMatch, and, true),
            'cq-targeting-mastertitleid-nor': isMultipleValueMatch(isMasterTitleIdTargetMatch, or, true),
            'cq-targeting-mastertitleid-not': not(isMasterTitleIdTargetMatch)
        },
        GamesDataFactory = null,
        SubscriptionFactory = null;


        return {
            init: init,
            waitForDataSourcesReady: waitForDataSourcesReady,
            targetingItemMatches: targetingItemMatches
        };
    }


    function CQTargetingFactorySingleton(ObjectHelperFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('CQTargetingFactory', CQTargetingFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories['cq-targeting']Factory
     
     * @description
     *
     * Factory that takes either a JSON or XML response and parses out the targeting information
     */
    angular.module('origin-components')
        .factory('CQTargetingFactory', CQTargetingFactorySingleton);
}());