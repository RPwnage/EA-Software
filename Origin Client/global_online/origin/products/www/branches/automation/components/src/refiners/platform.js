/**
 * @file refiners/platform.js
 */
(function(){

    'use strict';

    function PlatformRefiner(DateHelperFactory, GameRefiner) {
        var isSameDate = DateHelperFactory.isSameDate,
            addDays = DateHelperFactory.addDays,
            subtractDays = DateHelperFactory.subtractDays,
            isBetweenDates = DateHelperFactory.isBetweenDates,
            isInThePast = DateHelperFactory.isInThePast,
            platformToFacetKeyMap = {
                PCWIN: 'PC Download',
                MAC: 'Mac Download'
            },
            installablePlatformsMap = ['PCWIN', 'MAC'];

        /**
         * get the release date from a given catalog entry
         * @param {Object} catalogInfo
         * @param {string} platform the platform id (eg. PCWIN)
         * @return {Date}
         */
        function getReleaseDate(catalogInfo, platform) {
            return _.get(catalogInfo, ['platforms', platform, 'releaseDate']);
        }

        /**
         * get the download start date
         * @param {Object} catalogInfo
         * @param {string} platform the platform id (eg. PCWIN)
         * @return {Date}
         */
        function getDownloadStartDate(catalogInfo, platform) {
            return _.get(catalogInfo, ['platforms', platform, 'downloadStartDate']);
        }

        /**
         * Get the use end date (catalog item has expired)
         * @param {Object} catalogInfo
         * @param {string} platform the platform id (eg. PCWIN)
         * @return {Date}
         */
        function getUseEndDate(catalogInfo, platform) {
            return _.get(catalogInfo, ['platforms', platform, 'useEndDate']);
        }

        /**
         * Get the originSubscriptionUseEndDate field from the catalog
         * @param {Object} catalogInfo
         * @param {string} platform the platform id (eg. PCWIN)
         * @return {Date}
         */
        function getSubscriptionUseEndDate(catalogInfo, platform) {
            return _.get(catalogInfo, ['platforms', platform, 'originSubscriptionUseEndDate']);
        }

        /**
         * Map over the selected platform release dates for the given catalogInfo
         * @param  {Objsect} catalogInfo
         * @return {Function}
         */
        function mapPlatformReleaseDates(catalogInfo) {
            /**
             * For each platform get the release date
             * @param  {string} platform
             * @return {Object}
             */
            return function(platform) {
                return getReleaseDate(catalogInfo, platform);
            };
        }

        /**
         * Compare release dates for the given platforms
         * @param  {Array} platforms the platform list to compare
         * @return {Function}
         */
        function hasMatchingPlatformReleaseDates(platforms) {
            /**
             * map over platforms for matching dates
             * @param  {catalogInfo} catalogInfo
             * @return {Boolean}
             */
            return function(catalogInfo) {
                var data = platforms.map(mapPlatformReleaseDates(catalogInfo));

                if(data.length === platforms.length) {
                    for(var i = 0; i < data.length; i++) {
                        if(!isSameDate(data[0], data[i])) {
                            return false;
                        }
                    }

                    return true;
                }

                return false;
            };
        }

        /**
         * Is there a future release date set for this game and is it under a year from now?
         * @param {string} platform the platform identifier (eg. PCWIN)
         * @param {Number} withinDays specify how recently the game was released in days
         * @return {Function}
         */
        function isReleaseDateImminentInDays(platform, withinDays) {
            /**
             * @param {Object} catalogInfo the catalog model
             * @return {Boolean}
             */
            return function(catalogInfo) {
                var releaseDate = getReleaseDate(catalogInfo, platform),
                    thresholdDate = subtractDays(releaseDate, withinDays);

                return isBetweenDates(thresholdDate, releaseDate);
            };
        }

        /**
         * Is the catalog item release date within the specified days
         * @param {string} platform the platform identifier (eg. PCWIN)
         * @param {Number} withinDays specify how recently the game was released in days
         * @return {Function}
         */
        function isRecentlyReleasedInDays(platform, withinDays) {
            /**
             * @param {Object} catalogInfo the catalog model
             * @return {Boolean}
             */
            return function(catalogInfo) {
                var releaseDate = getReleaseDate(catalogInfo, platform),
                    thresholdDate = addDays(releaseDate, withinDays);

                return isBetweenDates(releaseDate, thresholdDate);
            };
        }

        /**
         * Is the catalog item released?
         * @param {string} platform the platform identifier (eg. PCWIN)
         * @return {Function}
         */
        function isReleased(platform) {
            /**
             * @param {Object} catalogInfo the catalog model
             * @return {Boolean}
             */
            return function(catalogInfo) {
                return isInThePast(getReleaseDate(catalogInfo, platform));
            };
        }

        /**
         * Is the catalog item released on any platform?
         * @return {Function}
         */
        function isReleasedOnAnyPlatform() {
            /**
             * @param {Object} catalogInfo the catalog model
             * @return {Boolean}
             */
            return function(catalogInfo) {
                var releasedPlatforms = installablePlatformsMap.filter(function(platform) {
                    return isReleased(platform)(catalogInfo);
                });
                return releasedPlatforms.length > 0;
            };
        }

        /**
         * Is there a future preload event for the given game
         * @param {String} platform the current operating system name from origin.utils.os
         * @return {Function}
         */
        function isPreloadImminentInDays(platform, withinDays) {
            /**
             * Compare the preload date in catalog  the current date
             * @param {Object} catalogInfo the catalog data model
             * @return {Boolean}
             */
            return function(catalogInfo) {
                var downloadStartDate = getDownloadStartDate(catalogInfo, platform),
                    thresholdDate = subtractDays(downloadStartDate, withinDays);

                return isBetweenDates(thresholdDate, downloadStartDate);
            };
        }

        /**
         * Is this game ready for preload?
         * @param {string} platform the platform identifier (eg. PCWIN)
         * @return {Function}
         */
        function isPreloadable(platform) {
            /**
             * Check if the given catalog record has a release date entry
             * And that the user has not taken action to install the game
             * @param {Object} catalogInfo the catalog data model
             * @param {Object} clientInfo the client data model
             * @param {Booean} isPreorder true if the game is still designated as a preorder (something went wrong in the billing backend)
             * @return {Boolean}
             */
            return function(catalogInfo, clientInfo, isPreorder) {
                var downloadStartDate = getDownloadStartDate(catalogInfo, platform),
                    releaseDate = getReleaseDate(catalogInfo, platform);

                return (
                    isBetweenDates(downloadStartDate, releaseDate) &&
                    !_.get(clientInfo, 'installed') &&
                    !isPreorder
                ) ? true : false;
            };
        }

        /**
         * Does this game exist for the selected platform?
         * @param {string} platform the platform identifier (eg. PCWIN)
         * @return {Function}
         */
        function isAvailableOnPlatform(platform) {
            /**
             * @param {Object} catalogInfo
             * @return {Boolean}
             */
            return function(catalogInfo) {
                if(!_.isObject(catalogInfo) || installablePlatformsMap.indexOf(platform) === -1) {
                    return false;
                }

                var platformFacetKey = _.get(catalogInfo, 'platformFacetKey');

                if(platformFacetKey) {
                    return (
                        GameRefiner.isHybridGame(catalogInfo) ||
                        platformFacetKey === platformToFacetKeyMap[platform]
                    ) ? true : false;
                }

                return (_.get(catalogInfo, ['platforms', platform, 'platform']) === platform);
            };
        }

        /**
         * Is this game expiring with the specified number of days on the specified platform?
         * @param {string} platform the platform identifier (eg. PCWIN)
         * @param {Number} withinDays the number of days away from the use end date
         * @return {Function}
         */
        function isExpiryImminentInDays(platform, withinDays) {
            /**
             * @param {Object} catalogInfo
             * @return {Boolean}
             */
            return function(catalogInfo) {
                if(!_.isObject(catalogInfo)) {
                    return false;
                }

                var useEndDate = getUseEndDate(catalogInfo, platform);

                return isBetweenDates(subtractDays(useEndDate, withinDays), useEndDate);
            };
        }

        /**
         * Is this game expired?
         * @param {string} platform the platform identifier (eg. PCWIN)
         * @return {Function}
         */
        function isExpired(platform) {
            /**
             * @param {Object} catalogInfo
             * @return {Boolean}
             */
            return function(catalogInfo) {
                if(!_.isObject(catalogInfo)) {
                    return false;
                }

                return isInThePast(getUseEndDate(catalogInfo, platform));
            };
        }

        /**
         * Has the game been preloaded?
         * @param {string} platform the platform identifier (eg. PCWIN)
         * @return {Function}
         */
        function isPreloaded(platform) {
            /**
             * Check if the given catalog record has a release date entry
             * And that the user has taken action to install the game before the release date
             * @param {Object} catalogInfo
             * @param {Object} clientInfo
             * @return {Boolean}
             */
            return function(catalogInfo, clientInfo) {
                var downloadStartDate = getDownloadStartDate(catalogInfo, platform),
                    releaseDate = getReleaseDate(catalogInfo, platform);

                return (isBetweenDates(downloadStartDate, releaseDate) && _.get(clientInfo, 'installed') === true) ? true : false;
            };
        }

        /**
         * Check if a vault game is being removed from the vault within the specified number of days
         * @param {string} platform the platform identifier (eg. PCWIN)
         * @param {number} withinDays the number of days to check
         * @return {Function}
         */
        function isVaultGameExpiryImminentInDays(platform, withinDays) {
            /**
             * @param  {Object} catalogInfo the catalog data
             * @return {Boolean}
             */
            return function(catalogInfo) {
                var subscriptionUseEndDate = getSubscriptionUseEndDate(catalogInfo, platform);

                return isBetweenDates(subtractDays(subscriptionUseEndDate, withinDays), subscriptionUseEndDate);
            };
        }

        /**
         * Check if a vault game is being removed from the vault within the specified number of days
         * @param {string} platform the platform identifier (eg. PCWIN)
         * @return {Function}
         */
        function isVaultGameExpired(platform) {
            /**
             * @param  {Object} catalogInfo the catalog data
             * @return {Boolean}
             */
            return function(catalogInfo) {
                return DateHelperFactory.isInThePast(getSubscriptionUseEndDate(catalogInfo, platform));
            };
        }

        /**
         * Does the game have an alternative mac edition
         * @param {string} platform the platform identifier (eg. PCWIN)
         * @return {Function}
         */
        function hasVaultMacAlternative(platform) {
            /**
             * @param  {Object} catalogInfo the catalog data
             * @param {Object} isInSubscriptionAndActive the entitlement is a subscription entitlement and in good standing
             * @param {Boolean} isSubscriptionActive is the user an active subscriber
             * @return {Boolean}
             */
            return function(catalogInfo, isInSubscriptionAndActive, isSubscriptionActive) {
                if (!isInSubscriptionAndActive || !isSubscriptionActive || platform !== 'MAC') {
                    return false;
                }

                if (GameRefiner.isPurchasable(catalogInfo) && isAvailableOnPlatform(platform)(catalogInfo)) {
                    return true;
                }

                return false;
            };
        }

        return {
            hasMatchingPlatformReleaseDates: hasMatchingPlatformReleaseDates,
            isReleaseDateImminentInDays: isReleaseDateImminentInDays,
            isRecentlyReleasedInDays: isRecentlyReleasedInDays,
            isReleased: isReleased,
            isReleasedOnAnyPlatform: isReleasedOnAnyPlatform,
            isPreloadImminentInDays: isPreloadImminentInDays,
            isPreloadable: isPreloadable,
            isAvailableOnPlatform: isAvailableOnPlatform,
            isExpiryImminentInDays: isExpiryImminentInDays,
            isExpired: isExpired,
            isPreloaded: isPreloaded,
            isVaultGameExpiryImminentInDays: isVaultGameExpiryImminentInDays,
            isVaultGameExpired: isVaultGameExpired,
            hasVaultMacAlternative: hasVaultMacAlternative
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.refiners.PlatformRefiner

     * @description
     *
     * Helpful methods for filtering content based on platform
     */
    angular.module('origin-components')
        .factory('PlatformRefiner', PlatformRefiner);
})();