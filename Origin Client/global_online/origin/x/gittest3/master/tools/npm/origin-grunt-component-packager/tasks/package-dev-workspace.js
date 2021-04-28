/**
 * Create the components needed to populate the defaults upon import
 *
 * @author Richard Hoar <rhoar@ea.com>
 */

'use strict';

module.exports = function(grunt, options) {
    var path = require('path'),
        devDesignBuilder = require(__dirname + '/lib/cq-build-design.js')(),
        componentBag = [];

    /**
     * Initialize the workspace
     *
     * @param {string} workspaceName the friendly name for the workspace
     * @param {string} encoding the encoding to use for XML
     */
    function init(workspaceName, encoding) {
        devDesignBuilder.init(workspaceName, encoding);
    }

    /**
     * Write design data
     */
    function writeDesignData() {
        devDesignBuilder.addComponent('development', componentBag);
        grunt.file.write(path.normalize(options.packageDestinationPath + '/jcr_root' + options.devDesignCrxPath + '/.content.xml'), devDesignBuilder.asXml());
    }

    /**
     * Add a component to the list of allowed components
     *
     * @param {string} path the component's CRX path
     */
    function addComponentPath(path) {
        componentBag.push(path);
    }

    return {
        init: init,
        addComponentPath: addComponentPath,
        writeDesignData: writeDesignData
    };
};
