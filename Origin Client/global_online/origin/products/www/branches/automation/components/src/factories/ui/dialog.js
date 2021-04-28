/**
 * @file common/dialog.js
 */
(function() {
    'use strict';

    function DialogFactory($document, $rootScope, $compile, $timeout, AppCommFactory, StackFactory, ComponentsLogFactory, UtilFactory, NavigationFactory) {
        var dialog,
            returnVals = {
                id: '',
                accepted: false,
                content: {}
            },
            dialogInfoStack = StackFactory.make(),
            currentlyShowingDialog = null,
            htmlEscapes = {
                "'": '&#39;',
                '`': '&#96;'
            };

        function addToStack(conf){

            var haveIt = dialogInfoStack.contains(function(item) {
                    return item.hasOwnProperty('id') && item.id === conf.id;
                });

            if (!haveIt){
                //push it on the dialog stack
                dialogInfoStack.push(conf);
            }
        }

        function removeFromStack(id) {
            dialogInfoStack.remove(function(item) {
                return item.hasOwnProperty('id') && item.id === id;
            });
        }

        /**
         * notify the dialog directive that we want to show the UI
         * @param  {object} conf the object containing the dialog info we want to show
         */
        function showDialog(conf) {
            currentlyShowingDialog = conf;
            dialog = $document.find('origin-dialog');
            if (dialog) {
                //tells the dialog directive to show itself
                AppCommFactory.events.fire('dialog:show', conf);
            }
        }

        /**
         * Checks if a dialog configuration object has neccessary information to open a dialog
         * A conf must be an object with either of these 2 fields and has proper data under those fields
         * directives: {object[]} an array of object containg the directive information to open
         * xmlDirective: {string} a string of html definition of the directive
         * @param {object} conf
         * @returns {boolean}
         */
        function isValidConf(conf) {
            var directives = _.get(conf, 'directives'),
                firstDirectiveObject = _.isArray(directives) && directives[0],
                xmlDirective = _.get(conf, 'xmlDirective');

            if (!_.isString(_.get(firstDirectiveObject, 'name', null)) && !_.isString(xmlDirective)) {
                return false;
            }
            return true;
        }

        /**
         * notify the dialog directive that we want to hide the UI
         * @param  {object} conf the object containing the dialog info we want to hide
         */
        function hideDialog(conf, shouldClose) {
            if (dialog) {
                returnVals.id = conf.id;
                returnVals.shouldClose = shouldClose;
                returnVals.stackSize = dialogInfoStack.length();

                //tells the dialog directive to hide itself
                AppCommFactory.events.fire('dialog:hide', returnVals);

                if (shouldClose) {
                    if (conf.callback) {
                        //trigger the callback assiged to this dialog with the return values
                        conf.callback(returnVals);
                    }
                }

                // Reset values
                currentlyShowingDialog = null;
                setReturnValues(false, {});
                returnVals.id = '';
            }
        }

        /**
         * pushes a dialog on to our dialog stack and show the dialog if in the SPA or ask the
         * OIG to show the dialog if in OIG
         * @param {Object} conf - configuration object
         * @return {void}
         */
        function open(conf) {
            if (!isValidConf(conf)){
                ComponentsLogFactory.log('DialogFactory:Invalid Conf Object');
                return;
            }

            // Add this dialog to the stack if it's not already there
            // We need to always add the dialog to the stack to handle
            // the edge case where the OIG SPA isn't initialized yet and
            // this dialog was triggered from within OIG.
            addToStack(conf);

            //if we are already showing a dialog, hide it
            if (currentlyShowingDialog) {
                if (currentlyShowingDialog.hasOwnProperty('id') && currentlyShowingDialog.id === conf.id) {
                    // It's possible to have the same dialog request to be shown esp within the OIG context
                    // If this is the case we don't want to do anything with the dialogs
                    // We do want to double check that the OIG SPA is open so the user can see every dialog
                    // This covers the case when the user closes the OIG SPA without closing the dialog

                    try {
                        if (Origin.client.oig.IGOIsAvailable()) {
                            NavigationFactory.openIGOSPA(conf.spalocation, conf.sublocation);
                        }
                    } catch(err) {
                        ComponentsLogFactory.log(err);
                    }

                    return;
                }
                // if the current dialog and our requested dialog are not the same hide the current one
                hideDialog(currentlyShowingDialog, false);
            }

            //if we are an OIG window, make a request to the C++ OIG to show this in the OIG SPA
            //when the OIG SPA Is ready it will trigger openPendingDialog (which shows the dialog)
            try {
                if (Origin.client.oig.IGOIsAvailable()) {
                    NavigationFactory.openIGOSPA(conf.spalocation, conf.sublocation);
                    return;
                }
            } catch(err) {
                ComponentsLogFactory.log(err);
            }

            showDialog(conf);
        }

        /**
         * closes the currently visible dialog and opens the next dialog in the stack if there is one
         * @param {String} id - id of a dialog to close if it's visible.
         * @return {void}
         */
        function close(id) {
            if (!id) {
                ComponentsLogFactory.error('This dialog was closed without passing an id');
            }

            // remove this dialog from the stack
            removeFromStack(id);

            // if this is the currently showing dialog hide it then show the next on the stack
            if (currentlyShowingDialog && currentlyShowingDialog.hasOwnProperty('id') && currentlyShowingDialog.id === id) {
                hideDialog(currentlyShowingDialog, true);
                if (dialogInfoStack.length()) {
                    showDialog(dialogInfoStack.peek());
                }
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
                    directives: [directiveInfo.contentDirective],
                    id: directiveInfo.id
                }],
                callback: directiveInfo.callback,
                spalocation: directiveInfo.spalocation
            });
        }

        /**
         * Helper - Opens a set of elements in a paginated carousel
         * @param  {Object} directiveInfo All the info required to create the directive
         */
        function openPaginated(directiveInfo) {
            open({
                id: directiveInfo.id,
                size: directiveInfo.size ? directiveInfo.size : "medium",
                directives: [{
                    name: directiveInfo.name,
                    data: directiveInfo.data,
                    directives: directiveInfo.contentDirective
                }],
                callback: directiveInfo.callback,
                spalocation: directiveInfo.spalocation
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
                size: dialogInfo.size ? dialogInfo.size : "medium",
                blocking: dialogInfo.blocking,
                directives: [{
                    name: 'origin-dialog-content-prompt',
                    data: {
                        header: dialogInfo.title,
                        description: dialogInfo.description,
                        yesbtntext: dialogInfo.acceptText,
                        yesenabled: dialogInfo.acceptEnabled,
                        nobtntext: dialogInfo.rejectText,
                        defaultbtn: dialogInfo.defaultBtn,
                        blocking: dialogInfo.blocking,
                        id: dialogInfo.id
                    },
                    directives: [dialogInfo.contentDirective]
                }],
                callback: dialogInfo.callback,
                spalocation: dialogInfo.spalocation,
                sublocation: dialogInfo.sublocation
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
                size: dialogInfo.size ? dialogInfo.size : "medium",
                blocking: dialogInfo.blocking,
                directives: [{
                    name: 'origin-dialog-content-prompt',
                    data: {
                        header: dialogInfo.title,
                        description: dialogInfo.description,
                        yesbtntext: '',
                        yesenabled: true,
                        nobtntext: dialogInfo.rejectText,
                        defaultbtn: 'no',
                        blocking: dialogInfo.blocking,
                        id: dialogInfo.id
                    },
                    directives: [dialogInfo.contentDirective]
                }],
                callback: dialogInfo.callback,
                spalocation: dialogInfo.spalocation
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

        /**
         * open the dialog thats at the top of the stack
         * @return {[type]} [description]
         */
        function openPendingDialog() {
            if (currentlyShowingDialog) {
                showDialog(currentlyShowingDialog);
            } else {
                if (dialogInfoStack.length()) {
                    showDialog(dialogInfoStack.peek());
                }
            }
        }

        /**
         * Encode objects and arrays as JSON - this is unique to dialogs as the components have an
         * explicit JSON.parse step in them to handle json serialized data from C++
         * @param  {mixed} value the value to handle
         * @return {string} the string value of the passed value
         */
        function valuesToJson(value) {
            if(_.isArray(value) || _.isObject(value)) {
                return JSON.stringify(value);
            }

            return value;
        }

        /**
         * A custom escaper/string-izer for dialogs that does not include escaping html start end tags, ampersands and double quotes
         * @param {string} text the value to sanitize
         * @return {string} the sanitized and quoted string value
         */
        function escape(text) {
            return '\'' + UtilFactory.replaceAll(text, htmlEscapes) + '\'';
        }

        /**
         * The tag system has to allow for html to be embedded in the attributes so this
         * tag system is incompatible with the traditional dom document parser. The only way
         * to circumvent this behavior is to rewrite the dialog handler or build the tags as strings
         * and let Angular's specialized dom parser deal with them as done here.
         * @param  {string} elementName the HTML element name
         * @param  {Object} attributes a list of attributes as key:value pairs
         * @return {string} an HTML opening tag
         */
        function tagStart(elementName, attributes) {
            var attributeStrings = [elementName];

            _.forEach(attributes, function(value, key) {
                if (angular.isDefined(value) && !_.isNull(value)) {
                    attributeStrings.push([key, escape(value)].join('='));
                }
            });

            return ['<', attributeStrings.join(' '), '>'].join('');
        }

        /**
         * Close the tag
         * @param  {string} elementName the HTML element name
         * @return {string} an HTML close tag
         */
        function tagEnd(elementName) {
            return ['</', elementName, '>'].join('');
        }

        /**
         * Creates an HTML string with a passed in directive object.
         * We pass only Origin Client/GEO created information to dialogs, so the content can include html
         * tags, special characters and JSON payloads. Dialog really should be redesigned to shore
         * up these issues.
         * @param {Object[]} data array of directive objects.
         * @param {Array} currentNode the recursive node
         * @return {string} trusted value represented as a string
         *
         * TODO: We should prefer a traditional dom document buider over this custom XML serializer, but it will break the
         * implementation of dialogs as they are.
         */
        function buildHtml(data, currentNode) {
            if(!currentNode) {
                currentNode = [];
            }

            _.forEach(data, function(dialogNode) {
                if(_.get(dialogNode, 'name')) {
                    currentNode.push(tagStart(_.get(dialogNode, 'name'), _.mapValues(_.get(dialogNode, 'data'), valuesToJson)));

                    if(_.isArray(_.get(dialogNode, 'directives'))) {
                        _.forEach(_.get(dialogNode, 'directives'), function(directive) {
                            buildHtml([directive], currentNode);
                        });
                    }

                    currentNode.push(tagEnd(_.get(dialogNode, 'name')));
                }
            });

            return currentNode.join('');
        }

        return {
            open: open,
            close: close,
            update: update,
            createDirective: createDirective,
            openDirective: openDirective,
            openPrompt: openPrompt,
            openAlert: openAlert,
            setReturnValues: setReturnValues,
            openPaginated: openPaginated,
            openPendingDialog: openPendingDialog,
            buildHtml: buildHtml
        };
    }


    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function DialogFactorySingleton($document, $rootScope, $compile, $timeout, AppCommFactory, StackFactory, ComponentsLogFactory, UtilFactory, NavigationFactory, SingletonRegistryFactory) {
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
