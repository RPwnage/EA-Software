  /**
   * @file models/pinrecommendationnews.js
   */
  (function() {
      'use strict';
      //fallback for max number of recs
      var MAX_RECS = 100000,
          DEFAULT_NUM_RECS = 12;

      function PinRecommendationNewsFactory() {
          /**
           * gets the ctid
           * @param  {object} bucketNewsArticleItem one news article from the bucket
           * @return {string} the ctid of the news article item (which is the unique identifier)
           */
          function getCtid(bucketNewsArticleItem) {
              //there's always just one item in the feedData
              return _.get(bucketNewsArticleItem, ['feedData', '0', 'ctid']);
          }

          /**
           * check if the ctid matches the ctid of a bucketnewsArticleItem
           * @param  {string} ctid the ctid of the news article item (which is the unique identifier)
           * @param  {object} bucketNewsArticleItem one news article from the bucket
           * @return {boolean} true if match false if not
           */
          function matchCtids(ctid, bucketNewsArticleItem) {
              return getCtid(bucketNewsArticleItem) === ctid;
          }

          /**
           * given a ctid return the bucketNewsArticleItem
           * @param  {object[]} bucketConfig an array of configs for a bucket (each news article is an entry in the array)
           * @param  {string} recommendedItemCtid the ctid of the news article item (which is the unique identifier)
           * @return {object} bucketNewsArticleItem one news article from the bucket
           */
          function mapCtidToBucketNewsArticleItem(bucketConfig, recommendedItemCtid) {
              return _.find(bucketConfig, _.partial(matchCtids, recommendedItemCtid));
          }

          /**
           * add tracking and position info if the news article is using pin recommendations
           * @param  {object} recoResponse the response from the reco api
           * @param  {object} bucketNewsArticleItem one news article from the bucket
           * @param  {number} index the absolute position of the tile in the bucket
           * @return {object} a new object of the news article with the tracking info 
           */
          function applyTrackingInfo(recoResponse, bucketNewsArticleItem, index) {
              //if we using pin reco, clone it deep to and add tracking info as well as priority to match
              //the order coming out of pin
              if (bucketNewsArticleItem.usePinRec) {
                  var newBucketItem = _.cloneDeep(bucketNewsArticleItem),
                      feedData = _.get(newBucketItem, ['feedData', '0']) || {},
                      recs = _.get(recoResponse, ['recommendations', '0']) || {};

                  //these are set so that when buttons are clicked or impressions are encountered we can send this info
                  //these will get passed as attributes to the directive
                  feedData.recoid = feedData.ctid;
                  feedData.recotag = _.get(recs, ['track', 'trackingTag']) || '';
                  feedData.recoindex = index;
                  feedData.recotype = Origin.pin.constants.NEWS;

                  //this is used to set the order when its run through discover story factory
                  newBucketItem.priority = _.get(recs, ['result', 'item_list', 'length'], MAX_RECS) - index;

                  //means this bucket item only will have one tile
                  newBucketItem.limit = 1;

                  //means we will not decrease the priority
                  newBucketItem.diminish = 0;

                  //if there was a default position set, clear it since we are using priority
                  if (newBucketItem.position) {
                      delete newBucketItem.position;
                  }
                  return newBucketItem;
              } else {

                  //not a pin rec so just pass the object back
                  return bucketNewsArticleItem;
              }
          }

          /**
           * check if item is part of pin recommendation
           * @param  {object} bucketNewsArticleItem one news article from the bucket
           * @return {boolean} true if is part of pin recommendation, false if not
           */
          function checkNotUsingPinRec(bucketNewsArticleItem) {
              return !_.get(bucketNewsArticleItem, 'usePinRec');
          }

          /**
           * take the tracking info from the reco response and add to a filtered list of news articles
           * @param  {object[]} bucketConfig an array of configs for a bucket (each news article is an entry in the array)
           * @param  {object} recoResponse the response from the reco api
           * @param  {object[]} filteredNewsWithTracking an array of filtered configs for a bucket (each news article is an entry in the array)
           */
          function applyTrackingToFilteredNews(bucketConfig, recoResponse) {
              var recs = _.get(recoResponse, ['recommendations', '0', 'result', 'item_list']) || [],
                  filteredNews = _.map(recs, _.partial(mapCtidToBucketNewsArticleItem, bucketConfig)),
                  filteredNewsWithTracking = _.map(filteredNews, _.partial(applyTrackingInfo, recoResponse));

              return filteredNewsWithTracking;
          }

          /**
           * concatenates the recommended and hand curated articles into one array
           * @param  {object[]} bucketConfig an array of configs for a bucket (each news article is an entry in the array)
           * @param  {object[]} filteredNewsWithTracking an array of filtered configs for a bucket (each news article is an entry in the array)
           * @return {object[]}} an array of news articles containing filtered news articles and handcrafted articles by geo
           */
          function addNonPinRecArticlesBackToCollection(bucketConfig, filteredNewsWithTracking) {
              var notPinRecNews = _.filter(bucketConfig, checkNotUsingPinRec);
              return filteredNewsWithTracking.concat(notPinRecNews);
          }

          /**
           * takes in an array of new article configs, runs it through the reco engine and returns a filtered list
           * @param  {object[]} bucketConfig an array of configs for a bucket (each news article is an entry in the array)
           * @return {object[]} a filtered list of news article objects
           */
          function filterRecommendedNews(bucketConfig, numrecs) {
              //first filter out the news articles that are will be inputs to the pin reco engine, then create an array of
              //ctids as input to the reco api
              var pinRecNews = _.filter(bucketConfig, 'usePinRec'),
                  ctids = _.map(pinRecNews, getCtid),
                  maxRecs = numrecs || DEFAULT_NUM_RECS;

              if (ctids.length) {
                  return Origin.pin.getNewsRecommendation(ctids, maxRecs)
                      .then(_.partial(applyTrackingToFilteredNews, bucketConfig))
                      .then(_.partial(addNonPinRecArticlesBackToCollection, bucketConfig));

              } else {
                  //if we don't have any articles for pin reco, just pass back the entire list (no filter)
                  return Promise.resolve(bucketConfig);
              }
          }

          return {
              /**
               * takes in an array of new article configs, runs it through the reco engine and returns a filtered list
               * @param  {object[]} bucketConfig an array of configs for a bucket (each news article is an entry in the array)
               * @return {object[]} a filtered list of news article objects
               */
              filterRecommendedNews: filterRecommendedNews
          };
      }

      /**
       * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
       * @param {object} SingletonRegistryFactory the factory which manages all the other factories
       */
      function PinRecommendationNewsFactorySingleton(SingletonRegistryFactory) {
          return SingletonRegistryFactory.get('PinRecommendationNewsFactory', PinRecommendationNewsFactory, this, arguments);
      }

      /**
       * @ngdoc service
       * @name origin-components.factories.PinRecommendationNewsFactory
       
       * @description
       *
       * PinRecommendationNewsFactory
       */
      angular.module('origin-components')
          .factory('PinRecommendationNewsFactory', PinRecommendationNewsFactorySingleton);
  }());