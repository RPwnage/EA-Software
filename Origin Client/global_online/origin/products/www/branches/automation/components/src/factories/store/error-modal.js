/**
 * @file store/error-modal.js
 */
(function() {
    'use strict';

    function ErrorModalFactory(DialogFactory) {

        var DIALOG_ID = 'error-modal-dialog';

        /**
         * Show error dialog based on errorCode
         * @param  {string} errorCode error string identifier
         */
        function showDialog(errorCode, replace) {

            if (errorCode && errorCode === 'login-aborted') {
                return;
            }

            // window object can be null if OIG window is closed
            if (!window) {
                return;
            }

            // ObjectSerializerFactory serialize/deserialize is not suitable for this
            replace = encodeURI(JSON.stringify(replace || {}));
            DialogFactory.open({
                id: DIALOG_ID,
                xmlDirective: '<origin-dialog-errormodal class="origin-dialog-content" dialog-id="'+DIALOG_ID+'" error-code="'+errorCode+'" replace="'+replace+'"></origin-dialog-errormodal>',
                size: 'large'
            });
        }

        return {
            showDialog: showDialog
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.ErrorModalFactory

     * @description
     *
     * Store error modal factory
     */
    angular.module('origin-components')
        .factory('ErrorModalFactory', ErrorModalFactory);
}());
