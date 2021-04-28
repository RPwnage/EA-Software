(function() {
    'use strict';
    function OriginDialogContentProgressCtrl($scope) {
        /**
         * Update the progress
         * @return {void}
         */
        this.update = function(conf) {
            if(conf.contentType !== 'origin-dialog-content-progress') {
                return;
            }
            $scope.title = conf.content.title;
            $scope.operation = conf.content.operation;
            $scope.operationstatus = conf.content.operationstatus;
            $scope.percent = conf.content.percent;
            $scope.rate = conf.content.rate;
            $scope.timeremaining = conf.content.timeremaining;
            $scope.phase = conf.content.phase;
            $scope.completed = conf.content.completed;
            $scope.$digest();
        };
    }

    function originDialogContentProgress(ComponentsConfigFactory, AppCommFactory) {
        /**
        * Progress link function
        * @return {void}
        */
        function originDialogContentProgressLink(scope, elem, attrs, ctrl) {

            /**
            * Clean up after yoself
            * @method onDestroy
            * @return {void}
            */
            function onDestroy() {
                AppCommFactory.events.off('dialog:update', ctrl.update);
            }

            AppCommFactory.events.on('dialog:update', ctrl.update);
            scope.$on('$destroy', onDestroy);

        }
        return {
            restrict: 'E',
            transclude: true,
            scope: {
                title: '@',
                operation: '@',
                operationstatus: '@',
                percent: '@',
                rate: '@',
                timeremaining: '@',
                phase: '@',
                completed: '@'
            },
            controller:'OriginDialogContentProgressCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/progress.html'),
            link: originDialogContentProgressLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentProgress
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * Dialog content that displays operation progress
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dialog-content-progress></origin-dialog-content-progress>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originDialogContentProgress', originDialogContentProgress)
        .controller('OriginDialogContentProgressCtrl', OriginDialogContentProgressCtrl);
}());