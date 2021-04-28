/**
 * @file common/dialog.js
 */
(function() {
    'use strict';

    function DialogFactory($document, $rootScope, $compile, $timeout, AppCommFactory) {
        var dialog;
        var returnVals = {
            id: '',
            accepted: false,
            content : {}
        };

        /**
         * Create the dialog and add it to the body
         * @return {void}
         */
        function createDialog() {
            var $body = $document.find('body'),
                scope = $rootScope.$new();
            dialog = $compile('<origin-dialog></origin-dialog>')(scope);
            $body.append(dialog);
        }

        /**
         * Open the dialog
         * @param {Object} conf - configuration object
         * @return {void}
         */
        function open(conf) {
            if (!dialog) {
                console.error('DialogFactory - not ready. Trying again.');
                $timeout(open, 1000);
            } else {
                AppCommFactory.events.fire('dialog:show', conf);
            }
        }


        /**
         * Close the dialog
         * @param {String} id - id of a dialog to close if it's visible.
         * @return {void}
         */
        function close(id) {
            if (dialog) {
                returnVals.id = id;
                AppCommFactory.events.fire('dialog:hide', returnVals);
                // Reset values
                setReturnValues(false, {});
                returnVals.id = '';
            }
        }

        /**
         * Updates the current dialog
         * @param {Object} dialogInfo - configuration object
         * @return {void}
         */
        function update(config) {
            if (dialog) {
                AppCommFactory.events.fire('dialog:update', config);
            }
        }


        /**
         * Helper - Opens a basic directive template
         * @param {Object} single directive info that will be shown
         * id  {String} - Unique ID of dialog
         * name  {String} - name of directive
         * data  {Object} - directive data
         * contentDirective  {Object} - singular content of directive
         * @return {Object}
         */
        function openDirective(directiveInfo) {
            open({
                id: directiveInfo.id,
                size: directiveInfo.size ? directiveInfo.size : "medium",
                directives: [{
                    name: directiveInfo.name,
                    data: directiveInfo.data,
                    directives: [directiveInfo.contentDirective]
                }]
            });
        }


        /**
         * Helper - Creates a basic directive template
         * @param {String} directive name
         * @param {Object} directive data
         * @param {Object} another directive - this will be embedded
         * @return {Object}
         */
        function createDirective(directiveName, directiveData, contentDirective) {
            return {
                name: directiveName,
                data: directiveData,
                directives: [contentDirective]
            };
        }


        /**
         * Helper - Opens a prompt template
         * @param {Object} Parameters for creating a basic dialog template.
         * id - {String} dialog id
         * title - {String} prompt title
         * text - {String} prompt text
         * acceptText - {String} button accept text
         * rejectText - {String} button reject text
         * defaultBtn - {String} default button ('yes'/'no')
         * contentDirective - {Object} content directive
         */
        function openPrompt(dialogInfo) {
            open({
                id: dialogInfo.id,
                directives: [
                {
                    name: 'origin-dialog-content-prompt',
                    data: {
                        header: dialogInfo.title,
                        description: dialogInfo.description,
                        yesbtntext: dialogInfo.acceptText,
                        yesenabled: dialogInfo.acceptEnabled,
                        nobtntext: dialogInfo.rejectText,
                        defaultbtn: dialogInfo.defaultBtn
                    },
                    directives: [dialogInfo.contentDirective]
                }
                ]
            });
        }


        /**
         * Helper - Opens an alert template
         * @param {Object} Parameters for creating a basic dialog template.
         * id - {String} dialog id
         * title -  {String} prompt title
         * text - {String} prompt text
         * rejectText - {String} button reject text
         * contentDirective - {Object} content directive
         */
        function openAlert(dialogInfo) {
            open({
                id: dialogInfo.id,
                directives: [
                {
                    name: 'origin-dialog-content-prompt',
                    data: {
                        header: dialogInfo.title,
                        description: dialogInfo.description,
                        yesbtntext: '',
                        yesenabled: true,
                        nobtntext: dialogInfo.rejectText,
                        defaultbtn: 'no'
                    },
                    directives: [dialogInfo.contentDirective]
                }
                ]
            });
        }
        
        /**
         * Sets the values that will be returned to C++ when current dialog is closed.
         * @param {Boolean} If content was accepted
         * @param {Object} Object to pass back to C++
         * @return {void}
         */
        function setReturnValues(accepted, content) {
            returnVals.accepted = accepted;
            returnVals.content = content;
        }

        // create the dialog on document ready
        $document.ready(createDialog);

        return {

            /**
             * Open the dialog
             * @param {Object} conf - configuration object
             * @return {void}
             */
            open: open,
            close: close,
            update: update,
            createDirective: createDirective,
            openDirective: openDirective,
            openPrompt: openPrompt,
            openAlert: openAlert,
            setReturnValues: setReturnValues
        };
    }


    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function DialogFactorySingleton($document, $rootScope, $compile, $timeout, AppCommFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('DialogFactory', DialogFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.DialogFactory
     
     * @description
     *
     * DialogFactory
     */
    angular.module('origin-components')
        .factory('DialogFactory', DialogFactorySingleton);

}());