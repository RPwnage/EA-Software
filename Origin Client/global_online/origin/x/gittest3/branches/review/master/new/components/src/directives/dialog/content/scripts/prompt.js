(function() {
    'use strict';
    function OriginDialogContentPromptCtrl($scope, AppCommFactory, DialogFactory) {
        var contentDataFunction = null;
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
            DialogFactory.close();
        }

        /**
         * Update the prompt content
         * @return {void}
         */
        function updatePriv(conf) {
            $scope.yesenabled = conf.acceptEnabled;
        }

        /**
        * Clean up after yoself
        * @method onDestroy
        * @return {void}
        */
        function onDestroy() {
            AppCommFactory.events.on('dialog:update', updatePriv);
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
        // Prompt content directive needs to be able to access finish()
        this.finish = finishPriv;

        AppCommFactory.events.on('dialog:update', updatePriv);
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
                }).bind(this);
            }

            $btnNo.focus(function() {
                togglePrimary($btnNo, $btnYes);
            }).bind(this);
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
                yesenabled: '@'
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