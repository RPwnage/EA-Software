/**
 * @file factories/gamelibrary/gametileviolator.js
 */
(function() {

    'use strict';

    var AUTOMATIC_TAG_ELEMENT_NAME = 'origin-game-violator-automatic',
        PROGRAMMED_TAG_ELEMENT_NAME = 'origin-game-violator-programmed';

    function GametileViolatorFactory(GameViolatorFactory, OcdHelperFactory, UtilFactory) {
        var buildTag = UtilFactory.buildTag,
            isValidViolatorData = GameViolatorFactory.isValidViolatorData;

        /**
         * Map the violator collection into concrete Dom tags for inline messages displayed in the header
         * @param {Object} data a violator record to analyze
         * @return {DomElement} the dom element for the directive
         */
        function mapInlineMessages(data, offerId) {
            if(data.violatorType === GameViolatorFactory.violatorType.automatic) {
                return buildTag(AUTOMATIC_TAG_ELEMENT_NAME, {
                    'violatortype': data.label,
                    'priority': data.priority,
                    'offerid': offerId,
                    'enddate': data.endDate || '',
                    'timeremaining': data.timeRemaining,
                    'timerformat': data.timerformat
                });
            } else {
                data.offerid = offerId;

                return OcdHelperFactory.wrapDocumentForCompile(buildTag(PROGRAMMED_TAG_ELEMENT_NAME, data));
            }
        }

        /**
         * Get the first n inline messages as straight up html
         * @param {Object[]} violatorData the input violator data feed
         * @param {String} offerId the offer id
         * @param {Number} inlineViolatorCount the maximum number of violators to show inline
         * @return {HTMLElement[]} a list of html elements
         */
        function getInlineMessages(violatorData, offerId, inlineViolatorCount) {
            if (isValidViolatorData(violatorData)) {
                return violatorData.map(_.partial(mapInlineMessages, _, offerId)).slice(0, inlineViolatorCount);
            }
        }

        return {
            getInlineMessages: getInlineMessages,
            getDialogMessages: _.noop,
            getReadMoreLinkHtml: _.noop
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GametileViolatorFactory

     * @description
     *
     * Helpers to build the violators message element(s) for the game tiles
     */
    angular.module('origin-components')
        .factory('GametileViolatorFactory', GametileViolatorFactory);
}());