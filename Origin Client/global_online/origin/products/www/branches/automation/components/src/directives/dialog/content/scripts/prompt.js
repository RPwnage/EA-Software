(function() {
    'use strict';

    /* jshint ignore:start */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    function OriginDialogContentPromptCtrl($scope, $timeout, AppCommFactory, DialogFactory) {
        var contentDataFunction = null,
            dialogUpdateHandle;
        $scope.yesenabled = ($scope.yesenabled === 'true');

        /**
         * Gets the data from prompt content directive, sets the return value, and closes dialog
         * @return {void}
         */
        function finishPriv(result) {
            var content = {};
            if(contentDataFunction) {
                content = contentDataFunction();
            }
            // OFM-11901: If the user pressed 'Yes' and if the can continue wasn't specified or is true, allow dialog to close.
            if(result === true && content.preventContinue === true) {
                console.error('Not closing window. Can not continue');
                return;
            }
            DialogFactory.setReturnValues(result, content);
            DialogFactory.close($scope.id);
        }

        /**
         * Update the prompt content
         * @return {void}
         */
        function updatePriv(conf) {
            $scope.yesenabled = conf.acceptEnabled;
            $timeout(function () {
                $scope.$digest();
            });
        }

        /**
        * Clean up after yoself
        * @method onDestroy
        * @return {void}
        */
        function onDestroy() {
            if(dialogUpdateHandle) {
                dialogUpdateHandle.detach();
            }
        }

        /**
         * Reaction from the 'otkmodal-btn-yes' button click. Closes dialog with accepted = true
         * @return {void}
         */
        $scope.onYesBtnClick = function() {
            finishPriv(true);
        };

        /**
         * Reaction from the 'otkmodal-btn-no' button click. Closes dialog with accepted = false
         * @return {void}
         */
        $scope.onNoBtnClick = function() {
            finishPriv(false);
        };

        /**
         * Allow content directive to see the content data. We need to be able to
         * how the user interacted with the data.
         * @param {Object} data - the data to store
         * @return {void}
         */
        this.setContentDataFunc = function(dataFunc) {
            contentDataFunction = dataFunc;
        };

        /**
        * Allow Prompts to handle key stroke events
        * @param {Object} event - key stroke event
        * @return {void}
        */
        this.handleButtonPress = function(event) {
            if ((event.keyCode || event.which) === 13) {
                event.stopImmediatePropagation();
                event.preventDefault();
                if($scope.defaultbtn === 'yes') {
                    finishPriv(true);
                } else {
                    finishPriv(false);
                }
            }
        };

        // Prompt content directive needs to be able to access finish()
        this.finish = finishPriv;

        dialogUpdateHandle = AppCommFactory.events.on('dialog:update', updatePriv);
        $scope.$on('$destroy', onDestroy);

    }

    function originDialogContentPrompt(ComponentsConfigFactory) {
        /**
        * Prompt link function
        * @return {void}
        */
        function originDialogContentPromptLink(scope, elem) {
            var $btnYes = $(elem.find('.otkmodal-btn-yes'));
            var $btnNo = $(elem.find('.otkmodal-btn-no'));
            var CLS_BTN_LIGHT = 'otkbtn-light';
            var CLS_BTN_PRIMARY = 'otkbtn-primary';

            var togglePrimary = function($focusBtn, $unfocusBtn) {
                if($focusBtn.length) {
                    $focusBtn.toggleClass(CLS_BTN_PRIMARY, true);
                    $focusBtn.toggleClass(CLS_BTN_LIGHT, false);
                }
                if($unfocusBtn.length) {
                    $unfocusBtn.toggleClass(CLS_BTN_LIGHT, true);
                    $unfocusBtn.toggleClass(CLS_BTN_PRIMARY, false);
                }
            };

            if (scope.defaultbtn === 'yes') {
                togglePrimary($btnYes, $btnNo);
            } else {
                togglePrimary($btnNo, $btnYes);
            }

            if($btnYes.length) {
                $btnYes.focus(function() {
                    togglePrimary($btnYes, $btnNo);
                });
            }

            $btnNo.focus(function() {
                togglePrimary($btnNo, $btnYes);
            });
        }
        return {
            restrict: 'E',
            transclude: true,
            scope: {
                header: '@', // Using header instead of title. 'Title' makes a tooltip appear.
                description: '@',
                yesbtntext: '@',
                nobtntext: '@',
                defaultbtn: '@',
                yesenabled: '@',
                blocking: '@',
                id: '@'
            },
            controller:'OriginDialogContentPromptCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/prompt.html'),
            link: originDialogContentPromptLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentPrompt
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} closeprompt desc
     * @param {LocalizedString} loginfailedtitle desc
     * @param {LocalizedString} loginfaileddesc desc
     * @param {LocalizedString} xmppuserconflicttitle desc
     * @param {LocalizedString} xmppuserconflictdesc desc
     * @param {LocalizedString} entitletmentserverdowntitle desc
     * @param {LocalizedString} entitletmentserverdowndesc desc
     * @param {LocalizedString} expiredcredentialtitle desc
     * @param {LocalizedString} expiredcredentialdesc desc
     * @param {LocalizedString} basegamenotfoundtitle base game not found title
     * @param {LocalizedString} basegamenotfounddesc base game not found desc
     * @param {BooleanEnumeration} blocking - "true" if the dialog is to be blocking
     *
     * Basic dialog prompt/alert template.
     * Easily created with DialogFactory.openAlert(...) / DialogFactory.openPrompt(...)
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dialog-content-prompt></origin-dialog-content-prompt>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originDialogContentPrompt', originDialogContentPrompt)
        .controller('OriginDialogContentPromptCtrl', OriginDialogContentPromptCtrl);
}());
