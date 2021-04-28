/**
 * @file home/priorityengine.js
 */

(function() {
    'use strict';

    function PriorityEngineFactory() {

        /**
         * used in a sort based on tilePriority
         */
        function comparePriority(a, b) {
            return b.tilePriority - a.tilePriority;
        }

        /**
         * used in a sort based on position
         */
        function comparePosition(a, b) {
            return a.position - b.position;
        }

        /**
         * creates a new story object
         * @param  {number} index the index into the feed array for the type
         * @param  {object} feedTypeConfig reference to the object that contains feed type data like directive name, feed array
         * @return {object} a new object that contains the index and a reference to the feedTypeConfig               
         */
        function createStoryObject(index, feedTypeConfig) {
            return {
                index: index,
                feedTypeConfig: feedTypeConfig
            };
        }

        /**
         * generate the priority based stories from config data
         * @param  {object} feedTypeConfig config data for the feed type
         * @return {object[]} a list of generated stories
         */
        function generatePriorityStoriesArray(feedTypeConfig) {
            var storiesAdded = 0,
                storiesArray = [],
                currentPriority = feedTypeConfig.priority,
                limit = feedTypeConfig.limit,
                diminish = feedTypeConfig.diminish;

            while (currentPriority) {
                //see if we've hit our limit or if the priority has been reduced to below zero
                if ((currentPriority < 0) || ((limit >= 0) && (storiesAdded >= limit))) {
                    currentPriority = 0;
                } else {

                    var storyObject = createStoryObject(storiesAdded, feedTypeConfig);
                    storyObject.tilePriority = currentPriority;
                    //add a story to show
                    storiesArray.push(storyObject);

                    //decrease priority by diminish value
                    currentPriority -= diminish;
                    storiesAdded++;

                }
            }
            //track the number of stories for this particular type
            //we'll need this when we want to add more tiles after initial generation
            //such as in the case of friend recommendations
            feedTypeConfig.storiesAdded = storiesAdded;
            return storiesArray;

        }

        /**
         * inserts stories into the correct position in an ordered stories array
         * @param  {object[]} positionfeedTypeConfigList list of position based config objects
         * @param  {object[]} storiesArray the story array you want to insert the stories to
         * @return {object[]} the stories array with the newly inserted position stories
         */
        function insertPositionStories(positionfeedTypeConfigList, storiesArray) {
            var i = 0;

            //sort the position type lists so that we can insert them in order
            positionfeedTypeConfigList.sort(comparePosition);

            //add each position based story to the correct spot in the overall story array
            for (i = 0; i < positionfeedTypeConfigList.length; i++) {
                storiesArray.splice(positionfeedTypeConfigList[i].position - 1, 0, createStoryObject(0, positionfeedTypeConfigList[i]));
            }

            return storiesArray;
        }

        /**
         * Generates a list of stories from a CMS config data
         * 
         * @param  {object} feedTypeConfigList an object of config data from the CMS
         * @return {object[]} a list of stories
         */
        function generateList(feedTypeConfigList) {
            var storiesArray = [],
                positionfeedTypeConfigList = [];

            //loop through our bucket config of different feed types
            for (var i = 0; i < feedTypeConfigList.length; i++) {
                //if its a position based feed type (a single tile that we insert into our list at a fixed position e.g. programmable tile)
                //we save it off for processing at the end
                if (feedTypeConfigList[i].position) {
                    positionfeedTypeConfigList.push(feedTypeConfigList[i]);
                } else {
                    //generate a list of stories for a specific feed type and add it to our stories list for the bucket
                    storiesArray.push.apply(storiesArray, generatePriorityStoriesArray(feedTypeConfigList[i]));
                }
            }

            //sort all the stories we have for the bucket so far by priority
            storiesArray.sort(comparePriority);

            //now lets insert all the stories that need to be a fixed positions into the stories for the bucket
            insertPositionStories(positionfeedTypeConfigList, storiesArray);

            //storiesArray now contains a correctly ordered list of stories for the bucket
            return storiesArray;
        }

        return {
            /**
             * Generates a list of stories from a CMS config data
             * 
             * @param  {object} feedTypeConfigList an object of config data from the CMS
             * @return {object[]} a list of stories
             */
            generateList: generateList
        };

    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function PriorityEngineFactorySingleton(SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('PriorityEngineFactory', PriorityEngineFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.PriorityEngineFactory
     
     * @description
     *
     * PriorityEngineFactory
     */
    angular.module('origin-components')
        .factory('PriorityEngineFactory', PriorityEngineFactorySingleton);
}());