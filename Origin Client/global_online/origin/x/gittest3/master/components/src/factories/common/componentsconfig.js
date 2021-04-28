/**
 * @file common/componentsconfig.js
 */
(function() {
    'use strict';

    function ComponentsConfigFactory() {
        var basePath = '',
            templatePath = 'dist/directives/',
            imagePath = 'dist/images/';
        return {
            /**
             * @ngdoc method
             * @name setBasePath
             * @methodOf origin-components.factories.ComponentsConfigFactory
             * @description
             *
             * setBasePath
             */
            setBasePath: function(path) {
                basePath = path;
            },

            /**
             * @ngdoc method
             * @name getTemplatePath
             * @methodOf origin-components.factories.ComponentsConfigFactory

             * @description
             *
             * getTemplatePath
             */
            getTemplatePath: function(template) {
                return basePath + templatePath + template;
            },
            /**
             * @ngdoc method
             * @name getImagePath
             * @methodOf origin-components.factories.ComponentsConfigFactory
             * @description
             *
             * getImagePath
             */
            getImagePath: function(image) {
                return basePath + imagePath + image;
            }
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.ComponentsConfigFactory
     * @description
     *
     * ComponentsConfigFactory
     */
    angular.module('origin-components')
        .factory('ComponentsConfigFactory', ComponentsConfigFactory);

}());