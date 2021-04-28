/**
 * @file refiners/entitlementstate.js
 */
(function(){

    'use strict';

    function EntitlementStateRefiner(DateHelperFactory, GameRefiner, PlatformRefiner) {
        var addDays = DateHelperFactory.addDays,
            subtractDays = DateHelperFactory.subtractDays,
            isInTheFuture = DateHelperFactory.isInTheFuture,
            isInThePast = DateHelperFactory.isInThePast,
            isValidDate = DateHelperFactory.isValidDate,
            isBetweenDates = DateHelperFactory.isBetweenDates,
            isBaseGame = GameRefiner.isBaseGame;


        /**
        * Determine if the game is expired
        * @param {object} entitlementInfo - the entitlement model
        * @param {Date} now
        * @return {Boolean}
        */
        function isExpired(entitlementInfo) {
            return isInThePast(_.get(entitlementInfo, 'terminationDate'));
        }

        /**
        * Determine if the game is active, the status can
        * be active, disabled, pending, deleted, banned.  If
        * it is anything but active, you basically can't play it
        * @param {object} entitlementInfo - the entitlement model
        * @return {Boolean}
        */
        function isActive(entitlementInfo) {
            return (_.get(entitlementInfo, 'status') === 'ACTIVE') ? true : false;
        }

        /**
        * Determine if the game is a preload
        * @param {object} entitlementInfo - the entitlement model
        * @return {Boolean}
        */
        function isPreorder(entitlementInfo) {
            return (_.get(entitlementInfo, 'entitlementTag') === 'ORIGIN_PREORDER') ? true : false;
        }

        /**
         * Determine if the preload failed to bill - this is a weird entitlement state to be in
         * and needs Customer Experience resolution if it happens
         * @param {string} platform the platform
         * @return {Function}
         */
        function isFailedPreorder(platform) {
            /**
             * @param  {Object}  catalogInfo the catalog info
             * @param  {Object}  entitlementInfo the entitlement info
             * @return {Boolean}
             */
            return function(catalogInfo, entitlementInfo) {
                return PlatformRefiner.isReleased(platform)(catalogInfo) && isPreorder(entitlementInfo);
            };
        }

        /**
        * Determine if the game is a part of Origin Access Subscription Offering
        * @param {object} entitlementInfo - the entitlement model
        * @return {Boolean}
        */
        function isInSubscription(entitlementInfo) {
            return (_.get(entitlementInfo, 'externalType') === 'SUBSCRIPTION') ? true : false;
        }

        /**
         * Is OA game expired?
         * @param {Object} entitlementInfo  the entitlement info
         * @param {Boolean} isActiveSubscription the user has an active subscription
         * @return {Function}
         */
        function isVaultGameExpired(entitlementInfo, isActiveSubscription) {
            return (isInSubscription(entitlementInfo) && !isActiveSubscription);
        }

        /**
         * Is the game in an update state?
         * @param {Object} clientInfo the client data model
         * @return {Boolean}
         */
        function isUpdateAvailable(clientInfo) {
            return (clientInfo.updateAvailable) ? true : false;
        }

        /**
         * Is the game for the wrong architecture?
         * @param {Object} clientInfo the client data model
         * @return {Boolean}
         */
        function isWrongArchitecture(clientInfo) {
            return (_.get(clientInfo, 'isWrongArchitecture') === true);
        }

        /**
         * Is a base game expiring in the future?
         * @param {Number} withinDays the number of days to show the expired message
         * @return {Function}
         */
        function isBaseGameExpiryImminentInDays(withinDays) {
            /**
             *   @param {Object} catalogInfo the catalog model
             * @param {Object} entitlementInfo the entitlement model
             * @return {Boolean}
             */
            return function(catalogInfo, entitlementInfo) {
                return (
                    isBaseGame(catalogInfo) &&
                    isValidDate(entitlementInfo.terminationDate) &&
                    isBetweenDates(subtractDays(entitlementInfo.terminationDate, withinDays), entitlementInfo.terminationDate)
                );
            };
        }

        /**
         * Is an addon/expansion awaiting download/install and granted within the sepcified number of days
         * @param {Number} withinDays the number of days since entitlement grantDate to compare
         * @return {Function}
         */
        function isAddonOrExpansionAwatingDownloadInDays(withinDays) {
            /**
             * @param {Object} clientInfo the client data model
             * @param {Object} entitlementInfo the entitlement model
             * @return {Boolean}
             */
            return function(clientInfo, entitlementInfo) {
                if ((clientInfo.downloadable || clientInfo.installable) &&
                    isValidDate(entitlementInfo.grantDate) &&
                    isInTheFuture(addDays(entitlementInfo.grantDate, withinDays))) {
                    return true;
                }

                return false;
            };
        }

        /**
         * Test if an addon or expansion has been installed in the specified number of days
         * @param {Number} withinDays the number of days since release to compare
         * @return {Function}
         */
        function isAddonOrExpansionRecentlyInstalledInDays(withinDays) {
            /**
             * @param {Object} clientInfo the client data model
             * @param {Date} baseGameLastPlayed the last played Date
             * @return {Boolean}
             */
            return function(clientInfo, baseGameLastPlayed) {
                var dlcInstallDate = new Date(_.get(clientInfo, 'initialInstalledTimestamp'));

                if( clientInfo.installed &&
                    isValidDate(dlcInstallDate) &&
                    isValidDate(baseGameLastPlayed) &&
                    dlcInstallDate > baseGameLastPlayed &&
                    isInTheFuture(addDays(dlcInstallDate, withinDays))
                ) {
                    return true;
                }

                return false;
            };
        }

        /**
         * Determine if the game is disabled/unplayable
         * @param {Object} catalogInfo the catalog info
         * @param {Object} entitlementInfo  the entitlement info
         * @param {Object} clientInfo the client info
         * @param {Boolean} isValidPlatform the user is on an Origin enabled platform
         * @param {Boolean} isTrialExpired the offer is a trial and is expired
         * @param {Boolean} isActiveSubscription the user has an active subscription
         * @return {Boolean}
         */
        function isDisabled(catalogInfo, entitlementInfo, clientInfo, isValidPlatform, isTrialExpired, isActiveSubscription) {
            var os = Origin.utils.os();

            return (
                isExpired(entitlementInfo) ||
                (isTrialExpired && (GameRefiner.isTrial(catalogInfo) || GameRefiner.isEarlyAccess(catalogInfo))) ||
                isVaultGameExpired(entitlementInfo, isActiveSubscription) ||
                (isInSubscription(entitlementInfo) && 
                    ((isActiveSubscription && PlatformRefiner.isVaultGameExpired(os)(catalogInfo)) ||
                    os === 'MAC')) ||
                isFailedPreorder(os)(catalogInfo, entitlementInfo) ||
                PlatformRefiner.isExpired(os)(catalogInfo) ||
                !(isActive(entitlementInfo) || _.get(clientInfo, 'playable') ||  _.get(clientInfo,'downloadable') ||  _.get(clientInfo, 'installable')) ||
                isWrongArchitecture(clientInfo) ||
                !(isValidPlatform && PlatformRefiner.isAvailableOnPlatform(os)(catalogInfo))
            ) ? true : false;
        }

        /**
         * Determine if the game is a download override
         * @param {Object} clientInfo the client info
         * @return {Boolean}
         */
        function isDownloadOverride(clientInfo) {
            return (_.get(clientInfo, 'hasDownloadOverride')) ? true : false;
        }

        /**
         * determines if an entitlement was granted under the "On The House" Program
         * @param  {Object} entitlementInfo the entitlement info
         * @return {Boolean} returns true if granted under the "On The House" Program
         */
        function isOTHGranted(entitlementInfo) {
            return (
                _.get(entitlementInfo, 'entitlementSource') === "Ebisu-Platform" &&
                (GameRefiner.isOnTheHouse(entitlementInfo) || GameRefiner.isNormalGame(entitlementInfo))
            ) ? true : false;
        }

        /**
         * Is the game actionable? Logic to show or hide the CTA.
         * @param {Object} catalogInfo the catalog info
         * @param {Object} entitlementInfo  the entitlement info
         * @param {Object} clientInfo the client info
         * @param {Boolean} isValidPlatform the user is on an Origin enabled platform
         * @param {Boolean} isActiveSubscription the user has an active subscription
         * @param {Boolean} isTrialExpired the user's trial has expired
         * @param {Boolean} isNog is the game a non origin game
         * @return {Boolean}
         */
        function isGameActionable(catalogInfo, entitlementInfo, clientInfo, isValidPlatform, isActiveSubscription, isTrialExpired, isNog, isPrivate) {
            var os = Origin.utils.os();

            // Preorder games are kind of weird in that they're not disabled game tiles, so this is an extension
            // Of the isDisabled logic and specific to whether a user can take action on the game.
            // Brief: it will never show a CTA for preload games that have already been successfully installed
            // Brief: it will never show a CTA for preordered games before their preload date
            if (!isNog &&
                (!PlatformRefiner.isReleased(os)(catalogInfo) && !isPrivate) &&
                !PlatformRefiner.isPreloadable(os)(catalogInfo, clientInfo, isPreorder(entitlementInfo))
            ) {
                return false;
            }

            return (
                isNog ||
                _.get(clientInfo, 'uninstallable') ||
                _.get(clientInfo, 'uninstalling') ||
                (isTrialExpired && (GameRefiner.isTrial(catalogInfo) || GameRefiner.isEarlyAccess(catalogInfo))) ||
                !isDisabled(catalogInfo, entitlementInfo, clientInfo, isValidPlatform, isTrialExpired, isActiveSubscription)
            ) ? true : false;
        }

        return {
            isActive: isActive,
            isExpired: isExpired,
            isPreorder: isPreorder,
            isFailedPreorder: isFailedPreorder,
            isInSubscription: isInSubscription,
            isUpdateAvailable: isUpdateAvailable,
            isWrongArchitecture: isWrongArchitecture,
            isBaseGameExpiryImminentInDays: isBaseGameExpiryImminentInDays,
            isAddonOrExpansionAwatingDownloadInDays: isAddonOrExpansionAwatingDownloadInDays,
            isAddonOrExpansionRecentlyInstalledInDays: isAddonOrExpansionRecentlyInstalledInDays,
            isDisabled: isDisabled,
            isDownloadOverride: isDownloadOverride,
            isVaultGameExpired: isVaultGameExpired,
            isOTHGranted: isOTHGranted,
            isGameActionable: isGameActionable
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.refiners.EntitlementStateRefiner

     * @description
     *
     * Helpful methods for filtering content based on entitlement, client and catalog constraints
     */
    angular.module('origin-components')
        .factory('EntitlementStateRefiner', EntitlementStateRefiner);
})();