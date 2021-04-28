/**
 * @file controltoast.js
 */
(function() {
    'use strict';

    /* jshint ignore:start */
    /**
     * ControlToastTypeEnumeration - enumeration of toast types
     * @enum {string}
     */
    var ControlToastTypeEnumeration = {
        "user": "user",
        "simple": "simple",
        "game": "game"
    };
    /* jshint ignore:end */

    angular.module('origin-components')
     /**
     * @ngdoc directive
     * @name origin-components.directives:originControlToast
     * @restrict E
     * @element ANY
     * @scope
     * @param {ControlToastTypeEnumeration} toasttype val : user (shows avatar), simple (shows icon), game (shows box art) : (no value is a text only toast)
     * @param {LocalizedString} toasttitle "This user is on your block list."
     * @param {LocalizedString} toastbody "If you would like to see more information about them, remove them from your"
     * @param {LocalizedString} toasticon val : class name of otkicon
     * @description
     *
     * Blocked Profile
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-control-toast
     *            toasttype="user"
     *            toasttitle="title"
     *            toastbody="body"
     *            toasticon="otkicon"
     *         ></origin-control-toast>
     *     </file>
     * </example>
     *
     */
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
