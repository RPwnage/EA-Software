(function() {
    'use strict';

    function ShellCtrl() {
        // shell has loaded hide the spinner
        document.getElementsByTagName('body')[0].classList.remove('origin-isloading');
    }

    /**
     * @ngdoc controller
     * @name originApp.controllers:ShellCtrl
     * @description
     *
     * The controller for the shell
     */
    angular.module('originApp')
        .controller('ShellCtrl', ShellCtrl);
}());
