/**
 * @file scripts/dialog.js
 */
(function() {
    'use strict';

    // constants
    var KEY_ESCAPE = 27;

    function OriginDialogCtrl($scope, $compile, ScopeHelper, AppCommFactory, UtilFactory, DialogFactory) {
        var newScope = null,
            DEFAULTS = {
                'modal': true,
                'size': 'medium',
                'blocking': false
            };

        $scope.isHidden = true;
        $scope.config = DEFAULTS;
        $scope.id = '';

        function addClassToDirectives(conf) {
            _.forEach(conf.directives, function(directive) {
                if (!_.get(directive, ['data', 'class'])) {
                    directive.data.class = 'origin-dialog-content';
                } else {
                    directive.data.class += 'origin-dialog-content';
                }
            });
        }

        /**
         * Show the dialog
         * @param {object} conf - the configuration object.  A sample configuration. MUST PASS UNIQUE ID
         *   object looks like
         *   {
         *       'modal': true, // can also be false
         *       'size': 'medium', // can also be large
         *       'blocking': true|false, // determines if this dialog is blocking or not. If not specified the dialog is NOT blocking
         *       'id': 'web-description', // unique id of dialog
         *       'xmlDirective': '<my-awesome-directive this=is my=attr></my-awesome-directive>' // XML tree of directives.
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
            $scope.isHidden = false;

            Origin.utils.mix($scope.config, conf);

            addClassToDirectives(conf);

            var tag = conf.xmlDirective || DialogFactory.buildHtml(conf.directives);

            // create a new scope and save it off so we can destroy
            // it manually and not leak memory
            newScope = $scope.$new();
            $scope.modalContent = $compile(tag)(newScope);
            $scope.showContent();
            if (!$scope.$$phase) {
                $scope.$digest();
            }

        };

        /**
         * Hide the dialog
         * @return {void}
         */
        this.hide = function() {
            AppCommFactory.events.fire('dialog:hidden');
            $scope.isHidden = true;
            ScopeHelper.digestIfDigestNotInProgress($scope);
        };

        /*
         * destroy scope so that the content gets destroyed
         * @return {void}
         */
        this.destroyScope = function() {
            if (newScope) {
                newScope.$destroy();
                newScope = null;
            }
        };
    }

    function originDialog(ComponentsConfigFactory, UtilFactory, AppCommFactory, ComponentsLogFactory, DialogFactory, $document, $window, ScreenFactory) {

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
                    if (timedHasFocus) {
                        scope.onClose(scope.id);
                    } else {
                        ComponentsLogFactory.error('Ignore ESC. Window not in focus.');
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
                return function() {
                    // Dialogs can be interacted with during the show transition so it's
                    // possible the user has already closed the dialog at this point.
                    if (scope.id !== dialogInfo.id) {
                        return;
                    }

                    // ORSTOR-5490: need to readjust the size after transition completes, this covers 
                    // switching from taller dialogs to shorter ones on mobile devices
                    adjustSize();

                    AppCommFactory.events.fire('cppDialog:showingDialog', {
                        id: dialogInfo.id
                    });
                };
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
                return function() {
                    if (dialogInfo.shouldClose) {
                        AppCommFactory.events.fire('cppDialog:finished', {
                            id: dialogInfo.id,
                            accepted: dialogInfo ? dialogInfo.accepted : false,
                            content: dialogInfo ? dialogInfo.content : {}
                        });
                    }
                    // It's possible that another dialog was shown in the time it took to play the hide animation.
                    // Only hide/destroy the dialog/backdrop if no other dialog was shown in the meantime.
                    if (scope.isHidden) {
                        modal.css('display', 'none');
                        backdrop.css('display', 'none');
                        //need to manually destroy the content, just clearing innerHTML doesn't actually destroy it
                        ctrl.destroyScope();
                        // When we are done showing the modal - clear the id
                        scope.id = '';
                    }
                };
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
                if (!conf.id) {
                    ComponentsLogFactory.error('dialog::show() arg.data didn\'t pass id - MUST PASS UNIQUE ID');
                    return;
                }
                if (!scope.isHidden && !scope.id) {
                    ComponentsLogFactory.log('Dialog ' + conf.id + ' already showing. Try again later');
                    return;
                }
                modal.css('display', 'block');
                backdrop.css('display', 'block');
                modal[0].offsetWidth; // jshint ignore:line
                backdrop[0].offsetWidth; // jshint ignore:line
                adjustSize();
                ctrl.show(conf);
                UtilFactory.onTransitionEnd(modal, onShowAnimationFinish({
                    id: scope.id
                }));

                ScreenFactory.disableBodyScrolling();
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
                if (retVals && retVals.id) {
                    ComponentsLogFactory.log('Trying to close dialog with id: ' + retVals.id + ' | scope id: ' + scope.id);
                    if (retVals.id !== scope.id) {
                        ComponentsLogFactory.error('not closing dialog. Ids don\'t match');
                        return;
                    }
                }

                ScreenFactory.enableBodyScrolling();
                angular.element($document).off('keyup', onKeyUp);
                angular.element($window).off('resize', adjustSize);
                angular.element($window).off('focus', onFocusIn);
                angular.element($window).off('blur', onFocusOut);
                ctrl.hide();

                // We need to call the transitionEnd function when the animation actually completes unless we are
                // showing a new dialog.  The endFunction does clean up of the dialog we which would break the new dialog
                //  being shown.
                var transitionEndFunction = onHideAnimationFinish({
                    id: scope.id,
                    accepted: retVals ? retVals.accepted : false,
                    content: retVals ? retVals.content : {},
                    shouldClose: retVals ? retVals.shouldClose : false
                });

                if(retVals && retVals.stackSize === 0) {
                    UtilFactory.onTransitionEnd(modal, transitionEndFunction);
                } else {
                    transitionEndFunction();
                }

            };

            scope.onClose = function(id) {
                DialogFactory.close(id);
            };

            /**
             * Listen for the backdrop to be clicked. If application is in focus, hide modal.
             * @return {void}
             * @method
             */
            scope.onBackdropClick = function() {
                if (timedHasFocus && !scope.config.blocking) {
                    DialogFactory.close(scope.id);
                }
            };

            AppCommFactory.events.on('dialog:show', scope.show);
            AppCommFactory.events.on('dialog:hide', scope.hide);
            scope.$on('$destroy', onDestroy);

            // Tell C++ that we are ready to show it's dialogs
            AppCommFactory.events.fire('cppDialog:dialogSystemReady');

            //see if there's any pending dialogs and try to show them. Some dialogs maybe have been requested before this directive has been loaded.
            DialogFactory.openPendingDialog();
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
