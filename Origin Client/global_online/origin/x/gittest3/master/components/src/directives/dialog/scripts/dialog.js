/**
 * @file scripts/dialog.js
 */
(function() {
    'use strict';

    // constants
    var KEY_ESCAPE = 27;

    function OriginDialogCtrl($scope, $compile, AppCommFactory, UtilFactory) {

        // defaults
        var DEFAULTS = {
            'modal': true,
            'size': 'medium'
        };

        $scope.isHidden = true;
        $scope.config = DEFAULTS;
        $scope.id = '';

        /**
         * Creates a tag with data as attributes passed to the
         * directive.
         * @param {Object} conf - the configuration object
         * @return {String} - the tag
         */
        function createTag(conf) {
            return UtilFactory.buildTag(conf.directives);
        }


        /**
         * Show the dialog
         * @param {object} conf - the configuration object.  A sample configuration. MUST PASS UNIQUE ID
         *   object looks like
         *   {
         *       'modal': true, // can also be false
         *       'size': 'medium', // can also be large
         *       'id': 'web-description', // unique id of dialog
         *       'directives': [{
         *            'name': 'directive name',
         *            'data': {}, // any data that you want to pass to the scope of the dialog
         *            'directives': [] // any embedded directives - using same directives model as parent.
         *        }]
         *   }
         *
         * DialogFactory provides helper functions for making common models:
         * var data = {
         *     redeemtitle: 'Label Text',
         *     redeemtext: 'Description.'
         *     };
         * var content = DialogFactory.createDirective('origin-dialog-content-rtpnotentitled', data);
         * DialogFactory.openAlert('UNIQUE ID', 'Title', 'Description', 'Cancel', content);
         * @return {void}
         */
        this.show = function(conf) {
                $scope.config = DEFAULTS;
                $scope.id = conf.id;
                Origin.utils.mix($scope.config, conf);
                $scope.isHidden = false;
                $scope.modalContent = $compile(createTag(conf))($scope);
                $scope.showContent();
        };

        /**
         * Hide the dialog
         * @return {void}
         */
        this.hide = function() {
            AppCommFactory.events.fire('dialog:hidden');
                $scope.isHidden = true;
        };
    }

    function originDialog(ComponentsConfigFactory, UtilFactory, AppCommFactory, $document, $window) {

        // link function
        function originDialogLink(scope, elm, attrs, ctrl) {

            var modal = angular.element(elm[0].querySelectorAll('.otkmodal')),
                backdrop = angular.element(elm[0].querySelectorAll('.otkmodal-backdrop')),
                content = angular.element(elm[0].querySelectorAll('.otkmodal-content')),
                hasFocus = true,
                timedHasFocus = true,
                timedHasFocusTimer;

            /**
             * Reaction to window event focus in and keeps track of focus state.
             * If we didn't have focus, we want the event reason for
             * coming into focus go first (onBackdropClicked)
             * @return {void}
             * @method
             */
            function onFocusIn() {
                hasFocus = true;
                timedHasFocusTimer = setTimeout(function() {
                    timedHasFocus = true;
                }, 300);
            }

            /**
             * Reaction to window event focus out and keeps track of focus state
             * @return {void}
             * @method
             */
            function onFocusOut() {
                hasFocus = false;
                timedHasFocus = false;
                clearTimeout(timedHasFocusTimer);
            }

            /**
             * Listen to events and close the modal
             * if the user presses escape
             * @param {Event} e - event object
             * @return {void}
             * @method
             */
            function onKeyUp(e) {
                if (e.keyCode === KEY_ESCAPE) {
                    if(timedHasFocus) {
                    scope.hide();
                    } else {
                        console.error('Ignore ESC. Window not in focus.');
                }
            }
            }

            /**
             * Adjust the size of the backdrop so that it facilities the modal
             * @return {void}
             * @method
             */
            function adjustSize() {
                backdrop
                    .css('height', 0)
                    .css('height', modal[0].scrollHeight + 'px');
            }

            /**
             * The callback of UtilFactory.onTransitionEnd. This is the result of after we are showing the dialog.
             * @param {Object} dialogInfo - Information from DialogFactory that we will send to the C++
             * - id {String} - the specific dialog we want to close
             * @return {void}
             */
            function onShowAnimationFinish(dialogInfo) {
                AppCommFactory.events.fire('cppDialog:showingDialog', {
                    id: dialogInfo.id,
                });
            }

            /**
             * The callback of UtilFactory.onTransitionEnd. This is the result of after we are finised showing the dialog.
             * @param {Object} dialogInfo - Information from DialogFactory that we will send to the C++
             * - id {String} - the specific dialog we want to close
             * - accepted {Boolean} - was the dialog accepted
             * - content {Object} - content being pass back
             * @return {void}
             */
            function onHideAnimationFinish(dialogInfo) {
                modal.css('display', 'none');
                backdrop.css('display', 'none');
                AppCommFactory.events.fire('cppDialog:finished', {
                    id: dialogInfo.id,
                    accepted: dialogInfo ? dialogInfo.accepted : false,
                    content: dialogInfo ? dialogInfo.content : {}
                });
                // When we are done showing the modal - clear the id
                scope.id = '';
            }

            /**
            * Clean up after yourself
            * @method onDestroy
            * @return {void}
            */
            function onDestroy() {
                AppCommFactory.events.off('dialog:show', scope.show);
                AppCommFactory.events.off('dialog:hide', scope.hide);
            }

            /**
             * Show the modal
             * @return {void}
             */
            scope.show = function(conf) {
                if(!conf.id) {
                    console.error(conf);
                    console.error('dialog::show() arg.data didn\'t pass id - MUST PASS UNIQUE ID');
                    return;
                }
                if(!scope.isHidden && !scope.id) {
                    console.log('Dialog ' + scope.id + ' already showing. Try again later');
                    return;
                }
                modal.css('display', 'block');
                backdrop.css('display', 'block');
                modal[0].offsetWidth; // jshint ignore:line
                backdrop[0].offsetWidth; // jshint ignore:line
                adjustSize();
                ctrl.show(conf);
                UtilFactory.onTransitionEnd(modal, onShowAnimationFinish({id: scope.id}));
                // Hide the Y title bar for the main page. If the window got
                // really small, there would be a double scroll bar
                $document.find('.otk').css('overflow-y', 'hidden');
                angular.element($document).on('keyup', onKeyUp);
                angular.element($window).on('resize', adjustSize);
                angular.element($window).on('focus', onFocusIn);
                angular.element($window).on('blur', onFocusOut);
            };

            /**
             * Add the modal content to the container and
             * pass any data down to the scope of the modal
             * @param {Object} data - *optional* any sort of data that you want to pass
             *   to the scope of the controller in the dialog content.
             * @return {void}
             */
            scope.showContent = function() {
                content.html('');
                content.append(scope.modalContent);
            };

            /**
             * Hide the modal
             * @param {Object} retVals - Information from DialogFactory that we will send to the C++
             * - id {String} - the specific dialog we want to close
             * - accepted {Boolean} - was the dialog accepted
             * - content {Object} - content being pass back
             * @return {void}
             */
            scope.hide = function(retVals) {
                if(retVals && retVals.id) {
                    console.log('Trying to close dialog with id: ' + retVals.id + ' | scope id: ' + scope.id);
                    if(retVals.id !== scope.id) {
                        console.error('not closing dialog. Ids don\'t match');
                        return;
                    }
                }
                $document.find('.otk').css('overflow-y', 'auto');
                angular.element($document).off('keyup', onKeyUp);
                angular.element($window).off('resize', adjustSize);
                angular.element($window).off('focus', onFocusIn);
                angular.element($window).off('blur', onFocusOut);
                ctrl.hide();
                UtilFactory.onTransitionEnd(modal, onHideAnimationFinish({
                    id: scope.id,
                    accepted: retVals ? retVals.accepted : false,
                    content: retVals ? retVals.content : {}
                }));
            };

            /**
             * Listen for the backdrop to be clicked. If application is in focus, hide modal.
             * @return {void}
             * @method
             */
            scope.onBackdropClick = function() {
                if(timedHasFocus) {
                    scope.hide();
                }
            };

            AppCommFactory.events.on('dialog:show', scope.show);
            AppCommFactory.events.on('dialog:hide', scope.hide);
            scope.$on('$destroy', onDestroy);

            // Tell C++ that we are ready to show it's dialogs
            AppCommFactory.events.fire('cppDialog:dialogSystemReady');
        }

        // directive declaration
        return {
            restrict: 'E',
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/views/dialog.html'),
            controller: 'OriginDialogCtrl',
            link: originDialogLink
        };
    }


    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialog
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * Container for the popup dialog
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dialog></origin-dialog>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginDialogCtrl', OriginDialogCtrl)
        .directive('originDialog', originDialog);
}());
