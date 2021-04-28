/**
 * @file controltoast.js
 */
(function() {
    'use strict';

    angular.module('origin-components')
    .directive('originControlToast', function(ComponentsConfigFactory) {
        return {
            restrict: "E",
            templateUrl: ComponentsConfigFactory.getTemplatePath('views/controltoast.html'),
            scope: {
                toasttype: "@", //val : user (shows avatar), simple (shows icon), game (shows box art) : (no value is a text only toast)
                toasttitle: "@",
                toastbody: "@",
                toasticon: "@", //val : class name of otkicon
            }
        };
    });
}());
