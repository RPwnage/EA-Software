/*jshint unused: false */
/*jshint undef:false */
/*jshint strict: false */

/**
 * this module handles all the communication with the C++ client
 */
define([
    'core/logger',
    'modules/client/ClientObjectWrapper',
    'modules/client/communication'
], function(logger, ClientObjectWrapper, communication) {

    var registryResolves = {};


    function createClientObject(objectName) {
        return new ClientObjectWrapper(objectName);
    }

    function createClientObjects() {
        for (var r in registryResolves) {
            if (registryResolves.hasOwnProperty(r)) {
                registryResolves[r](createClientObject(r));
            }
        }
        registryResolves = {};
    }    

    function registerClientObject(cppObjectName) {
        var promise = null;
        if (communication.getConnectionType() === communication.typeEnum.NOTCONNECTED) {
            promise = Promise.resolve(createClientObject(cppObjectName));
        } else {
            promise = new Promise(function(resolve) {
                registryResolves[cppObjectName] = resolve;
            });
        }

        return promise;
    }

    function init() {
        return communication.waitForConnectionEstablished()
            .then(createClientObjects);
    }

    return /** @lends module:Origin.module:client */ {
        registerClientObject: registerClientObject,
        init: init
    };
});