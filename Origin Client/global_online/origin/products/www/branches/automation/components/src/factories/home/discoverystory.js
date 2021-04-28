/**
 * @file home/discoverystory.js
 */
/* jshint latedef:nofunc */
(function() {
    'use strict';

    function DiscoveryStoryFactory(FeedFactory, PriorityEngineFactory, OcdHelperFactory, GamesDataFactory, ComponentsLogFactory) {

        /**
         * resolve the dependencies for a bucket
         * @param  {object} bucketConfig the configuration data for a given bucket
         * @return {promise} promise that resolves when all the dependencies have been retrieved
         */
        function getFeedData(bucketConfig) {
            var feedPromiseArray = [],
                feedPromise = null;

            for (var i = 0; i < bucketConfig.length; i++) {

                if (bucketConfig[i].feedFunctionName) {
                    feedPromise = FeedFactory[bucketConfig[i].feedFunctionName](bucketConfig[i]).catch(handleFeedError);
                } else {
                    feedPromise = Promise.resolve(bucketConfig[i].feedData || []);
                }

                feedPromiseArray.push(feedPromise);
            }

            return Promise.all(feedPromiseArray);
        }

        /**
         * logs and error and returns an empty array if there is a feed error
         * @param  {error} error the error object
         * @return {string[]]} empty array
         */
        function handleFeedError(error) {
            ComponentsLogFactory.error('[DiscoveryStoryFactory] retrieve feed data error', error);
            return [];
        }

        /**
         * save off the dependencies to the bucket config
         * @param  {object} bucketConfig configuration data for the bucket
         * @return {object} the bucketConfig object with the feed data added
         */
        function storeFeedData(bucketConfig) {
            return function(results) {
                for (var i = 0; i < bucketConfig.length; i++) {
                    bucketConfig[i].feedData = results[i];
                }

                return bucketConfig;
            };
        }

        /**
         * filter out objects with duplicate offerids
         * @param  {string[]} usedProductIds array of offerids already used
         * @return {function} function that takes in a game data feed item and check the offer if its already been used
         */
        function usedProductIdFilter(usedProductIds) {
            return function(gameDataFeedItem) {
                var keep = true,
                    offerId = gameDataFeedItem.offerid;
                if (offerId) {
                    //only keep if we have not already added a tile with this offer
                    keep = (usedProductIds.indexOf(offerId) < 0);

                    //we've haven't encountered this offer yet so add it to our used array
                    if (keep) {
                        usedProductIds.push(offerId);
                    }
                }
                return keep;

            };
        }

        /**
         * make sure offers do not show up twice in a bucket
         * @param  {object} bucketConfig  the bucket configuration
         * @return {object} the bucket config
         */
        function removeDuplicateOffersAndAdjustFeedLimits(bucketConfig) {
            var usedProductIds = [];
            bucketConfig.forEach(function(storyTypeConfig) {
                //filter out duplicate offers
                storyTypeConfig.feedData = storyTypeConfig.feedData.filter(usedProductIdFilter(usedProductIds));

                //adjust the story limits based on feed data available
                storyTypeConfig.limit = adjustStoryLimitsBasedOnAvailableFeedData(storyTypeConfig);
            });

            return bucketConfig;
        }

        /**
         * adjust the story limit for a given data type based on the amound of feed data. If we don't have any feed data for it
         * we shouldn't try to generate stories for it
         *
         * @param  {object} storyTypeConfig the config data for a particular story
         * @return {number} the adjusted limit
         */
        function adjustStoryLimitsBasedOnAvailableFeedData(storyTypeConfig) {
            var newLimit = storyTypeConfig.feedData.length,
                currentLimit = storyTypeConfig.limit;

            //adjust the length if its smaller or there was previously no limit set
            if (currentLimit === -1 || newLimit < currentLimit) {
                currentLimit = newLimit;
            }

            return currentLimit;
        }

        /**
         * calls an arbitrary FeedFactory function after initial story generation
         * @param  {object} bucketConfig config info for the bucket
         * @return {response} the response from the above promise chain (the story list)
         */
        function postGeneratedModifications(bucketConfig) {
            return function(response) {
                bucketConfig.forEach(function(storyTypeConfig) {
                    if (storyTypeConfig.postGenerationFeedModificationsFunctionName) {
                        FeedFactory[storyTypeConfig.postGenerationFeedModificationsFunctionName](storyTypeConfig);
                    }
                });

                return response;
            };

        }

        /**
         * generates stories from the config data from CMS
         * @param  {object} bucketConfig the config data for the bucket
         * @return {promise} a promise that returns a list of objects containing information about the indvidual tiles
         */
        function generateStories(bucketConfig) {
            return getFeedData(bucketConfig)
                .then(storeFeedData(bucketConfig))
                .then(removeDuplicateOffersAndAdjustFeedLimits)
                .then(PriorityEngineFactory.generateList)
                .then(postGeneratedModifications(bucketConfig));
        }

        /**
         * runs the  bucket config through a filter to remove any stories (in recommendation engine example)
         * @param  {object} bucketConfig the config data for the bucket
         * @param  {function} filterFunction  logic to apply to bucket config before  generating stories
         * @return {promise} a promise that returns a list of objects containing information about the indvidual tiles
         */
        function generateFilteredStories(bucketConfig, filterFunction) {
            if (filterFunction) {
                return filterFunction(bucketConfig)
                    .then(generateStories);
            } else {
                return generateStories(bucketConfig);
            }
        }

        /**
         * Use this callback if an OCD override is defined
         * @return {Function}
         */
        function slingDataToDomCallback(directiveName, attributesFromFeed) {
            /**
             * @param {mixed} data the OCD response to process
             * @return {Object} HTMLElement
             */
            return function(ocdData) {

                var ocdFormattedInfo = {};
                //default with the feed attribute
                ocdFormattedInfo[directiveName] = attributesFromFeed;

                //take any ocd and merge it in overwriting any feed data
                ocdFormattedInfo = _.merge(ocdFormattedInfo, ocdData);

                //pass this on to convert to dom markup
                return OcdHelperFactory.slingDataToDom(ocdFormattedInfo);
            };
        }

        /**
         * Retrieves OCD data and if an override exists, it is rendered, otherwise fall back to a default tak and offer id
         * with the same name as the story.
         * @param  {object} chosenStory contains the offerId and directive name
         * @return {Promise.<Object, Error>} The first tag
         */
        function buildDirective(configInfo) {
            var feedDataEntry = configInfo.feedTypeConfig.feedData[configInfo.index],
                //create a new object for the feed data so we can add to it
                attributes = _.merge({}, feedDataEntry);

            //if there is a child directive lets add it as an item to the current set of attributes
            if (configInfo.feedTypeConfig.childDirectives) {
                attributes.items = configInfo.feedTypeConfig.childDirectives;
            }

            //try to get ocd info and take the results and covert it to html markup
            return getOCDIfNeeded(configInfo.feedTypeConfig.directiveName, feedDataEntry.offerid)
                .then(slingDataToDomCallback(configInfo.feedTypeConfig.directiveName, attributes));
        }

        /**
        * Put the data into a wrapper object with the directive name as the key
        * @param {String} directiveName
        * @return {Function}
        */
        function stuffIntoDirectiveName(directiveName) {
            /**
            * @param {Object} data - data to stuff into wrapper
            * @return {Object}
            */
            return function(data) {
                var obj = {};
                obj[directiveName] = data;
                return obj;
            };
        }

        /**
         * get the OCD if there is a valid offerId
         * @param  {string} directiveName name of the directive
         * @param  {string} offerId offer id
         * @return {Promise.<Object>} returns a promise that resolves with ocd data if there is some
         */
        function getOCDIfNeeded(directiveName, offerId) {
            //if there is no
            if (!offerId) {
                return Promise.resolve();
            }
            return Promise.all([
                GamesDataFactory.getOcdByOfferId(offerId)
                    .then(OcdHelperFactory.findDirectives('origin-game-defaults'))
                    .then(OcdHelperFactory.flattenDirectives('origin-game-defaults'))
                    .then(_.head)
                    .then(stuffIntoDirectiveName(directiveName)),
                GamesDataFactory.getOcdByOfferId(offerId)
                    .then(OcdHelperFactory.findDirectives(directiveName))
                    .then(_.head)
                ])
                .then(_.spread(_.merge));
        }

        /**
         * generates markup from the story objects created by the generate functions function
         * this includes grabbing information from OCD and merging that in
         *
         * @param  {number} startIndex the starting index in the bucket we want to generate xml for
         * @return {number} numToGenerate the number of stories to generate xml for
         */
        function generateXML(startIndex, numToGenerate) {
            return function(generatedStories) {
                var markupList = [],
                    end = startIndex + numToGenerate;

                //if the number of tiles we are requesting is greated than the number of stories, adjust it
                if (end > generatedStories.length) {
                    numToGenerate = generatedStories.length - startIndex;
                    end = startIndex + numToGenerate;
                }

                //generate xml from the story data for a given range
                for (var i = startIndex; i < end; i++) {
                    markupList.push(buildDirective(generatedStories[i]));
                }

                return Promise.all(markupList);
            };
        }

        return {
            /**
             * generates stories from the config data from CMS
             * @param  {object} bucketConfig the config data for the bucket
             * @return {promise} a promise that returns a list of objects containing information about the indvidual tiles
             */
            generateStories: generateFilteredStories,
            /**
             * generates markup from the story objects created by the generate functions function
             * this includes grabbing information from OCD and merging that in
             *
             * @param  {number} startIndex the starting index in the bucket we want to generate xml for
             * @return {number} numToGenerate the number of stories to generate xml for
             */
            generateXML: generateXML
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.DiscoveryStoryFactory

     * @description
     *
     * DiscoveryStoryFactory
     */
    angular.module('origin-components')
        .factory('DiscoveryStoryFactory', DiscoveryStoryFactory);
}());
