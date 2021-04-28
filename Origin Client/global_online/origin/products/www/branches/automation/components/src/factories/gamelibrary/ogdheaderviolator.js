/**
 * @file factories/gamelibrary/ogdheaderviolator.js
 */
(function() {

    'use strict';

    var AUTOMATIC_TAG_ELEMENT_NAME = 'origin-gamelibrary-ogd-violator',
        PROGRAMMED_TAG_ELEMENT_NAME = 'origin-gamelibrary-ogd-important-info';

    function OgdHeaderViolatorFactory(GamesDataFactory, ObjectHelperFactory, GameViolatorFactory, OcdHelperFactory, DialogFactory) {
        var buildTag = OcdHelperFactory.buildTag,
            isValidViolatorData = GameViolatorFactory.isValidViolatorData;

        /**
         * Map the violator collection into an object usable by dialog's openDirective fn
         * @param  {Object} data the violator record to analyze
         * @return {Object} the dialog compatible object
         */
        function mapDialogMessages(data, offerId) {
            if (data.violatorType && data.violatorType === GameViolatorFactory.violatorType.automatic) {
                return {
                    name: AUTOMATIC_TAG_ELEMENT_NAME,
                    data: {
                        'violatortype': data.label,
                        'priority': data.priority,
                        'offerid': offerId,
                        'enddate': data.endDate ? data.endDate.toString() : '',
                        'timeremaining': data.timeRemaining,
                        'timerformat': data.timerformat
                    }
                };
            } else {
                data.offerid = offerId;

                return {
                    name: PROGRAMMED_TAG_ELEMENT_NAME,
                    data: data
                };
            }
        }

        /**
         * Map the violator collection into concrete Dom tags for inline messages displayed in the header
         * @param {Object} data a violator record to analyze
         * @return {DomElement} the dom element for the directive
         */
        function mapInlineMessages(data, offerId) {
            if (data.violatorType && data.violatorType === GameViolatorFactory.violatorType.automatic) {
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
         * Build a read more link that tells the user about more outstanding violators you have +15 violators
         * @param  {Number} totalMessageCount the violator total count
         * @param  {Number} inlineViolatorCount the number of inline violators shown
         * @param  {Function} viewmmoremessagesctaCallback A callback to fill in the pluralization template
         * @return {string} the HTML
         */
        function getReadMoreLinkHtml(totalMessageCount, inlineViolatorCount, viewmmoremessagesctaCallback) {
            var remainingMessagesCount = totalMessageCount - inlineViolatorCount;

            return [
                '<div class="origin-ogd-message origin-ogd-messages-readmore">',
                        '<a class="origin-ogd-messages-readmore-link" ng-click="readmore.show()">',
                            viewmmoremessagesctaCallback({'%messages%': remainingMessagesCount}, remainingMessagesCount),
                        '</a>',
                '</div>'
            ].join('');
        }

        /**
         * The read more dialog object
         * @param {String} gamemessages the string to put in the dialog header
         * @param {String} closemessagescta The text for the close button
         * @param {Object[]} dialogMessages the full list of dialog directives
         */
        function ReadMoreDialog(gamemessages, closemessagescta, dialogMessages) {
            this.gamemessages = gamemessages;
            this.closemessagescta = closemessagescta;
            this.dialogMessages = dialogMessages;
        }

        /**
         * Show the dialog containing all violators and important information
         */
        ReadMoreDialog.prototype.show = function () {
            var id = 'ogd-violator-read-more-dialog';
            DialogFactory.open({
                modal: true,
                size: 'medium',
                id: id,
                directives: [{
                    name: 'origin-dialog-content-prompt',
                    data: {
                        header: this.gamemessages,
                        nobtntext: this.closemessagescta,
                        defaultbtn: 'no',
                        id: id
                    },
                    directives: this.dialogMessages
                }]
            });
        };

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

        /**
         * Get the entire collection of messages in the dialog format
         * @param {Object[]} violatorData the input violator data feed
         * @param {String} offerId the offer id
         * @param {String} gamemessages the string to put in the dialog header
         * @param {String} closemessagescta The text for the close button
         * @return {Object[]} the data specific for the violator popup dialog
         */
        function getDialogMessages(violatorData, offerId, gamemessages, closemessagescta) {
            if (isValidViolatorData(violatorData)) {
                return new ReadMoreDialog(
                    gamemessages,
                    closemessagescta,
                    violatorData.map(_.partial(mapDialogMessages, _, offerId))
                );
            }
        }

        return {
            getInlineMessages: getInlineMessages,
            getDialogMessages: getDialogMessages,
            getReadMoreLinkHtml: getReadMoreLinkHtml
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.OgdHeaderViolatorFactory

     * @description
     *
     * Helpers to build the violators and overflow zones of the owned game details header
     */
    angular.module('origin-components')
        .factory('OgdHeaderViolatorFactory', OgdHeaderViolatorFactory);
}());

