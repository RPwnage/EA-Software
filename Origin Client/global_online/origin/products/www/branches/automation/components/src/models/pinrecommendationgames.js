  /**
   * @file models/pinrecommendationgames.js
   */
  (function() {
      'use strict';

      function PinRecommendationGamesFactory(GamesDataFactory, GamesEntitlementFactory, GamesCatalogFactory, StoreAgeGateFactory) {
          /**
           * converts an object of offers, to an array of offers based off the ordering from the ids array
           * @param  {string[]} ids array of offer ids
           * @return {object[]} an array of offers based off the ordering from the ids array
           */
          function mapOfferIdsToOffers(ids) {
              return GamesDataFactory.getCatalogInfo(ids)
                  .then(function(offers) {
                      return _.map(ids, function(item) {
                          return offers[item];
                      });
                  });
          }

          /**
           * generate a json object that is in the directive format that can be converted to xml
           * @param  {object} recoTrack the tracking information, such as tag, returned from the recommendation response
           * @param  {number} index     the absolute position of the recommendation in the array
           * @param  {object} catalogInfo catalog information
           * @param  {string} description description text
           * @return {object}           a json representing a directive that can be coverted to xml
           */
          function buildDirectiveObject(recoTrack, index, catalogInfo, description) {
              if (catalogInfo) {
                  return {
                      recotype: Origin.pin.constants.GAMES,
                      recoid: catalogInfo.projectNumber,
                      recoindex: index,
                      recotag: recoTrack.trackingTag,
                      offerid: catalogInfo.offerId,
                      description: description || '',
                      showprice: true,
                      items: [{
                          'origin-cta-downloadinstallplay': {
                              offerid: catalogInfo.offerId,
                              type: 'primary'
                          }
                      }]
                  };
              } 
              
              return {};
              
          }

          /**
           * Given the catalog info for a game, let it through if it passes the age gate
           * @param  {Object} catalogInfo game catalog info
           * @return {Object}             catalogInfo if age gate passed, else null
           */
          function filterByAgeGate(catalogInfo) {
              if(StoreAgeGateFactory.isUserUnderGameAgeRequirement(catalogInfo)) {
                  return null;
              }

              return catalogInfo;
          }

          /**
           * given a single recommendation (mastertitleid/projectid pair), find the best offer and build a json representing a directive that can be coverted to xml
           * @param  {object} recoTrack the tracking information, such as tag, returned from the recommendation response
           * @param  {object} recoItem  an indvidual recommended item (mastertitleid/projectid) pair
           * @param  {number} index     the absolute position of the recommendation in the array
           * @return {object}           a json representing a directive that can be coverted to xml
           */
          function getRecommendedOffer(recoTrack, recoItem, index) {
              var rankedOfferIds = _.map(GamesCatalogFactory.getOfferIdsFromMasterId(recoItem.masterTitleId), 'offerId');
              return mapOfferIdsToOffers(rankedOfferIds) //get catalog info for the ranked offer ids
                  .then(_.partial(GamesDataFactory.getPurchasableOfferByProjectId, recoItem.projectId)) //look for the best offer with matching projectId
                  .then(filterByAgeGate) // filter out games not appropriate for user's age
                  .then(_.partial(buildDirectiveObject, recoTrack, index)) //build the directive object; NOTE: do not show description for recommended games
                  .catch(function() {
                      return {}; // if no purchasable offer was found, return an empty directive object
                  });
          }

          /**
           * Given a control game (manually entered into CMS), construct a json representation of the game directive with reco tracking info
           * @param  {object} recoTrack   the tracking information, such as tag, returned from the recommendation response
           * @param  {object} controlItem an indvidual control game from CMS
           * @param  {number} index       the absolute position of the recommendation in the array
           * @return {object}             a json representing a directive that can be coverted to xml
           */
          function getControlOffer(recoTrack, controlItem, index) {
              var ocdPath = _.get(controlItem, ['attributes', 'ocd-path', 'value']),
                  description = _.get(controlItem, ['attributes', 'description', 'value']);

              return GamesDataFactory.lookUpOfferIdIfNeeded(ocdPath)
                  .then(function(offerId) {
                      return mapOfferIdsToOffers([offerId]);
                  })
                  .then(function(catalogInfo) {
                      return buildDirectiveObject(recoTrack, index, _.head(catalogInfo), description); // show description grabbed from manually entered control game
                  });
          }

          /**
           * takes the recommendation response array and transforms it to array of json representing directives that can be converted to xml
           * @param  {object} recoResponse the raw response from the reco api
           * @param  {object[]} controlGames array of objects representing <origin-home-discovery-tile-config-programmable-game>
           * @return {object[]} an array of objects in a format that can be converted to directives xml, representing the recommended games
           */
          function createRecommendedGamesJsonArray(recoResponse, controlGames) {
              //theres always going to be only one recommendation group
              var control = _.get(recoResponse, ['recommendations', '0', 'result', 'experimentData', 'control']) === 'yes', // whether user is in a control group
                  recoResults = _.get(recoResponse, ['recommendations', '0', 'result', 'item_list']) || [],
                  recoTrack = _.get(recoResponse, ['recommendations', '0', 'track']) || {},
                  recoMap;

              if (control) {
                  recoMap = _.map(controlGames, _.partial(getControlOffer, recoTrack));
              }
              else {
                  recoMap = _.map(recoResults, _.partial(getRecommendedOffer, recoTrack));
              }

              return Promise.all(recoMap)
                  .then(_.partialRight(_.filter, 'offerid'));
          }

          /**
           * based on the projectids from entitlements, we recommend the user a set of games to purchase
           * @param  {object} config object containing info related to 'recommended games', e.g, the max number of recommendations we are requesting, and control games
           * @return {object[]} an array of objects in a format that can be converted to directives xml, representing the recommended games
           */
          function retrieveRecommendedGamesToBuy(config) {
              var numrecs = config.numrecs;
              return GamesDataFactory.waitForInitialRefreshCompleted() //make sure we have initial entitlements
                  .then(GamesEntitlementFactory.getAllEntitlements) //get all entitlements
                  .then(_.partialRight(_.map, 'offerId')) //create an array of offer ids to pass to get catalog
                  .then(GamesDataFactory.getCatalogInfo) //get catalog info
                  .then(_.partialRight(_.map, 'projectNumber')) //create an array of project numbers to pass to reco API
                  .then(_.partialRight(Origin.pin.getGamesRecommendation, numrecs)) //call reco API
                  .then(_.partialRight(createRecommendedGamesJsonArray, config.controlGames));
          }

          /**
           * Filter out manually entered (in CMS) control games, which are marked by 'userPinRec'
           * @param  {object[]} bucketConfig array of objects representing <origin-home-discovery-tile-config-programmable-game>
           * @return {ojbect[]}              array of game objects without manually entered control games
           */
          function filterControlGames(bucketConfig) {
              _.remove(bucketConfig, 'usePinRec');
              return Promise.resolve(bucketConfig);
          }

          return {
              retrieveRecommendedGamesToBuy: retrieveRecommendedGamesToBuy,
              filterControlGames: filterControlGames
          };
      }

      /**
       * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
       * @param {object} SingletonRegistryFactory the factory which manages all the other factories
       */
      function PinRecommendationGamesFactorySingleton(GamesDataFactory, GamesEntitlementFactory, GamesCatalogFactory, StoreAgeGateFactory, SingletonRegistryFactory) {
          return SingletonRegistryFactory.get('PinRecommendationGamesFactory', PinRecommendationGamesFactory, this, arguments);
      }

      /**
       * @ngdoc service
       * @name origin-components.factories.PinRecommendationGamesFactory
       
       * @description
       *
       * PinRecommendationGamesFactory
       */
      angular.module('origin-components')
          .factory('PinRecommendationGamesFactory', PinRecommendationGamesFactorySingleton);
  }());