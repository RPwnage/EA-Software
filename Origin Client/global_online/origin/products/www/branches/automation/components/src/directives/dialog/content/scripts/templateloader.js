
/**
 * @file dialog/content/scripts/templateloader.js
 */
(function(){
    'use strict';
    
    function OriginDialogContentTemplateloaderCtrl($scope, TemplateResolver, $element, $compile, DialogFactory) {
        function closeModal() {
            DialogFactory.close($scope.modal);
        }

        function loadTemplate(template) {
            if(template) {
                //append the template
                $element.append(template);
                // then compile it so targeting still works
                $compile($element)($scope);
            } else {
                closeModal();
            }
        }

        TemplateResolver
            .getTemplate($scope.template, {})
            .then(loadTemplate)
            .catch(closeModal);
    }

    function originDialogContentTemplateloader(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                template: '@',
                modal: '@'
            },
            controller: OriginDialogContentTemplateloaderCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/templateloader.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentTemplateloader
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {String} template the cq5 template to load
     * @param {String} modal the ID of the modal
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-dialog-content-templateloader />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originDialogContentTemplateloader', originDialogContentTemplateloader);
}());
