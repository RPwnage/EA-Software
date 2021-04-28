/**
 * factory for an error logging service
 * @file common/componentscms.js
 */
(function() {
    'use strict';

    function ComponentsCMSFactory($http, ComponentsUrlsFactory, ComponentsLogFactory, UtilFactory) {

        function buildEndPoint(incomingUrl, feedType, sectionType, offerId, locale) {
                var endPoint = incomingUrl;
                if (endPoint) {
                    var setLocale;
                    if (typeof locale === 'undefined' || locale === '') {
                        setLocale = Origin.locale.locale();
                    } else {
                        setLocale = locale;
                    }

                    //we need to substitute
                    endPoint = endPoint.replace(/{feedType}/g, feedType); //replace all occurrences
                    endPoint = endPoint.replace('{locale}', setLocale);
                    endPoint = endPoint.replace('{sectionType}', sectionType);
                    if (offerId) {
                        endPoint += ('?offerId=' + offerId);
                    }

                } else {
                    ComponentsLogFactory.error('ERROR: cannot find url for story data');
                    endPoint = '';
                }
                return endPoint;
        }

        function formatResponse(offerId) {
            return function(response) {
                var retObj = {};

                response.replace('<?xml version=\"1.0\" encoding=\"UTF-8\"?>', '');
                
                retObj.xml = response;
                if(offerId) {
                    retObj.offerId = offerId;
                }

                return retObj;
            };
        }

        function retrieveStoryData(feedType, sectionType, offerId, locale) {
            return UtilFactory.getJsonFromUrl(buildEndPoint(ComponentsUrlsFactory.getUrl('homeStory'), feedType, sectionType, offerId, locale))
                .then(formatResponse(offerId));
        }

        return {
            retrieveDiscoveryStoryData: function(feedType, offerId, locale) {
                return retrieveStoryData(feedType, 'discoveries', offerId, locale);
            },
            retrieveRecommendedStoryData: function(feedType, offerId, locale) {
                return retrieveStoryData(feedType, 'actions', offerId, locale);
            }
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function ComponentsCMSFactorySingleton($http, ComponentsUrlsFactory, ComponentsLogFactory, UtilFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('ComponentsCMSFactory', ComponentsCMSFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.ComponentsCMSFactory
     * @requires $http
     * @description
     *
     * ComponentsCMSFactory
     */
    angular.module('origin-components')
        .factory('ComponentsCMSFactory', ComponentsCMSFactorySingleton);

}());