/**
* Helper for keyboard events
* @file helpers/keyevents.js
*/
(function() {
    'use strict';

    var KEYEVENT_MAP = {
       37: 'left',
       38: 'up',
       39: 'right',
       40: 'down',
       107: 'numpadPlus',
       109: 'numpadSubtract',
       13: 'enter',
       27: 'escape',
       8: 'backspace',
       9: 'tab',
       48: '0',
       49: '1',
       50: '2',
       51: '3',
       52: '4',
       53: '5',
       54: '6',
       55: '7',
       56: '8',
       57: '9',
       87: 'W',
       77: 'M'
    };


    function KeyEventsHelper() {

        /**
         * stop the propagation and prevent default so events don't bubble up
         * @param {Event} event the key event
         */
        function cancelEvent(event) {
            event.preventDefault();
            event.stopPropagation();
        }
        /**
        * Map they keycode to the action
        * @param {Event} event the key event
        * @param {Object} actionMap a map of the action name to the callback
        */
        function mapEvents(event, actionMap) {
            var keyCode = event.keyCode,
                actionName = keyCode ? KEYEVENT_MAP[keyCode] : false;
            if (actionName && actionMap[actionName]) {
                return actionMap[actionName](event);
            }
        }

        /**
        * Determine if the keycode pressed is not one that is involved with
        * entering characters of some sort
        * @param {Event} event
        * @return {Boolean}
        */
        function isNotCharacterCode(event) {
            var keyCode = event.keyCode,
                action = keyCode && KEYEVENT_MAP[keyCode],
                allowableActions = ['down', 'up', 'numpadPlus', 'numpadSubtract', 'escape', 'tab'];
            return allowableActions.indexOf(action) !== -1;
        }

        return {
            cancelEvent: cancelEvent,
            mapEvents: mapEvents,
            isNotCharacterCode: isNotCharacterCode
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.KeyEventsHelper
     * @description
     *
     * Key Events Helper
     */
    angular.module('origin-components')
        .factory('KeyEventsHelper', KeyEventsHelper);

}());
