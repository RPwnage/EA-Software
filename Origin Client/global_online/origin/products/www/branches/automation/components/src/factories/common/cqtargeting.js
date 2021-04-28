/**
 * @file cache.js
 */
(function() {
    'use strict';

    var SUBSCRIBER_TARGETING = {
        'true': 'true',
        'false': 'false',
        'usedsubstrial': 'usedsubstrial',
        'notusedsubstrial': 'notusedsubstrial'
    };

    function CQTargetingFactory(ObjectHelperFactory, DateHelperFactory, VaultRefiner, GamesDataFactory, GamesEntitlementFactory, SubscriptionFactory, AuthFactory) {

        /**
         * Convert the targetedValueStr to boolean and compare against the actual value
         * @param  {string} targetedValueStr the targeted value as a string
         * @param  {boolean} actualValue the actual value
         * @return {boolean} true if there is a match/false if there is not
         */
        function hasMatch(targetedValueStr, actualValue) {
            return (targetedValueStr.toString().toLowerCase() === 'true') === actualValue;
        }

        /**
         * check the users subscription trial state
         * @return {boolean}               true if trial has been consumed
         */
        function isSubscriptionFreeTrialConsumed() {
            return SubscriptionFactory.hasUsedFreeTrial();
        }

        /**
         * check the users subscription state
         * @param  {string} attributeValue the targeting attribute value
         * @return {boolean}               true if there's a match
         */
        function isSubscriberMatch(attributeValue) {
            return hasMatch(attributeValue, SubscriptionFactory.userHasSubscription());
        }

        /**
         * check the subscription state
         * @param  {string} attributeValue the targeting attribute value
         * @return {boolean}               true if there's a match
         */
        function isSubscriberTargetMatch(attributeValue) {
            switch(attributeValue) {
                case SUBSCRIBER_TARGETING.true:
                case SUBSCRIBER_TARGETING.false:
                    return isSubscriberMatch(attributeValue);
                case SUBSCRIBER_TARGETING.usedsubstrial:
                    return isSubscriptionFreeTrialConsumed(attributeValue);
                case SUBSCRIBER_TARGETING.notusedsubstrial:
                    return !isSubscriptionFreeTrialConsumed(attributeValue);
                default:
                    return true;
            }
        }

        /**
         * check if the subscriber has been active for less than daysActive
         * @param  {string} daysActive     the targeted days active
         * @return {boolean}               true if there's a match
         */
        function isSubscriptionActiveForLessThanNumDays(daysActive) {
            return VaultRefiner.isSubsActiveForLessThanNumDays(SubscriptionFactory.getfirstSignUpDate(), daysActive);
        }

        /**
         * check if the subscriber has been active for daysActive
         * @param  {string} daysActive     the targeted days active
         * @return {boolean}               true if there's a match
         */
        function isSubscriptionActiveForAtLeastNumDays(daysActive) {
            return VaultRefiner.isSubsActiveForAtLeastNumDays(SubscriptionFactory.getfirstSignUpDate(), daysActive);
        }

        /**
         * check if the subscriber has canceled but billing has NOT expired
         * @param  {string} attributeValue the targeting attribute value
         * @return {boolean}               true if there's a match
         */
        function isSubscriptionStatePendingExpired(attributeValue) {
            return hasMatch(attributeValue, SubscriptionFactory.isPendingExpired());
        }

        /**
         * check if the subscriber has canceled and billing expired
         * @param  {string} attributeValue the targeting attribute value
         * @return {boolean}               true if there's a match
         */
        function isSubscriptionStateExpired(attributeValue) {
            return hasMatch(attributeValue, SubscriptionFactory.isExpired());
        }

        /**
         * check if the subscriber was suspsended
         * @param  {string} attributeValue the targeting attribute value
         * @return {boolean}               true if there's a match
         */
        function isSubscriptionStateSuspended(attributeValue) {
            return hasMatch(attributeValue, SubscriptionFactory.isSuspended());
        }

        /**
         * check for ownership of an OTH title
         * @param  {string} attributeValue the targeting attribute value
         * @return {boolean}               true if there's a match
         */
        function isOTHTargetMatch(attributeValue) {
            return GamesDataFactory.ownsOTHEntitlementByPath(attributeValue);
        }


        /**
         * check if user is logged in
         * @param  {string} attributeValue the targeting attribute value
         * @return {boolean}               true if there's a match
         */
        function isUserLoggedIn(attributeValue) {
            return (attributeValue.toLowerCase() === 'default') || hasMatch(attributeValue, AuthFactory.isAppLoggedIn());
        }


        /**
         * check if user has no games
         * @param  {string} attributeValue the targeting attribute value
         * @return {boolean}               true if there's a match
         */
        function isEmptyShelf(attributeValue) {
            return hasMatch(attributeValue, !Object.keys(GamesEntitlementFactory.getAllEntitlements()).length);
        }        

        /**
         * check the underage state
         * @param  {string} attributeValue the targeting attribute value
         * @return {boolean}               true if there's a match
         */
        function isUnderageStateTargetMatch(attributeValue) {
            return hasMatch(attributeValue, Origin.user.underAge());
        }

        /**
         * check the offer path for ownership
         * @param  {string} attributeValue the targeting attribute value
         * @return {boolean}               true if there's a match
         */

        function isOcdPathTargetMatch(attributeValue) {
            return GamesDataFactory.ownsEntitlementByPath(attributeValue);
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
                var returnValue = logicFunction === and, // if our logic is and, we want our initial value to be true, other wise it will never be true
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
         * check to see if this target is based on entitlements
         * @param  {string}  attributeName the target attribute name
         * @return {Boolean} returns true if its an entitlement based target
         */
        function isEntitlementOwnershipTarget(attributeName) {
            return _.startsWith(attributeName, 'cq-targeting-ocdpath');
        }

        /**
         * check to see if we should ignore the target
         * @param  {string}  attributeName the target attribute name
         * @return {Boolean} returns true if its we should skip target
         */
        function skipTargetingItem(attributeName) {
            return isEntitlementOwnershipTarget(attributeName) && !AuthFactory.isAppLoggedIn();
        }

        /**
         * Check to see if we should show the tile. If the startdate has passed for tile. Currently if only one of the 
         * start or end date is configured the other one comes up as an empty string. For empty string usecase we need to treat
         * that as non-existant. It will return true for empty string to go ahead and reder the tile.
         * @param  {string} startDate start date for when the tile will be shown under MyHome
         * @return {boolean} true if start date has passed, false otherwise
         */
        function isPastStartTime(startDate) {
            return startDate ? DateHelperFactory.isInThePast(new Date(startDate)) : true;
        }

        /**
         * Check to see if we should hide the tile. If the enddate has passed for tile. Currently if only one of the 
         * start or end date is configured the other one comes up as an empty string. For empty string usecase we need to treat
         * that as non-existant. It will return true for empty string to go ahead and reder the tile.
         * @param  {string} endDate end date for when the tile will be removed from MyHome
         * @return {boolean} true if end date has passed, false otherwise
         */
        function isBeforeEndTime(endDate) {
            return endDate ? DateHelperFactory.isInTheFuture(new Date(endDate)) : true;
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

                        if(!skipTargetingItem(attributeName)) {
                            matched = matched && !!targetingAttributeMatches(attributeName, attributeValue);
                        }
                    }
                }, targetingObject);
            }

            return matched;
        }

        //the targeting attributes we look for
        var targetingConfigs = {
                'cq-targeting-subscribers': isSubscriberTargetMatch,
                'cq-targeting-subscribers-lapsed': isSubscriptionStateExpired,
                'cq-targeting-subscribers-pendingcancel': isSubscriptionStatePendingExpired,
                'cq-targeting-subscribers-suspended': isSubscriptionStateSuspended,
                'cq-targeting-subscribers-active-less': isSubscriptionActiveForLessThanNumDays,
                'cq-targeting-subscribers-active-more': isSubscriptionActiveForAtLeastNumDays,
                'cq-targeting-underage': isUnderageStateTargetMatch,
                'cq-targeting-loggedin': isUserLoggedIn,
                'cq-targeting-emptyshelf': isEmptyShelf,
                'cq-targeting-ocdpath': isOcdPathTargetMatch,
                'cq-targeting-starttime': isPastStartTime,
                'cq-targeting-endtime': isBeforeEndTime,
                'cq-targeting-ocdpath-and': isMultipleValueMatch(isOcdPathTargetMatch, and),
                'cq-targeting-ocdpath-or': isMultipleValueMatch(isOcdPathTargetMatch, or),
                'cq-targeting-ocdpath-nand': isMultipleValueMatch(isOcdPathTargetMatch, and, true),
                'cq-targeting-ocdpath-nor': isMultipleValueMatch(isOcdPathTargetMatch, or, true),
                'cq-targeting-ocdpath-not': not(isOcdPathTargetMatch),
                'cq-targeting-ocdpath-onthehouse-and': isMultipleValueMatch(isOTHTargetMatch, and),
                'cq-targeting-ocdpath-onthehouse-or': isMultipleValueMatch(isOTHTargetMatch, or),
                'cq-targeting-ocdpath-onthehouse-nand': isMultipleValueMatch(isOTHTargetMatch, and, true),
                'cq-targeting-ocdpath-onthehouse-nor': isMultipleValueMatch(isOTHTargetMatch, or, true)
            };


        return {
            targetingItemMatches: targetingItemMatches
        };
    }


    /**
     * @ngdoc service
     * @name origin-components.factories['cq-targeting']Factory

     * @description
     *
     * Factory that takes either a JSON or XML response and parses out the targeting information
     */
    angular.module('origin-components')
        .factory('CQTargetingFactory', CQTargetingFactory);
}());