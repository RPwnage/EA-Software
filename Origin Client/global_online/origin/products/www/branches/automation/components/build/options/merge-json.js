module.exports = function() {
    return {
        "config": {
            src: ["../tools/config/components-config.json", "../tools/config/environment/production/components-nonorigin-config.json"],
            dest: ".tmp/components-config.json"
        },
        "breakpoints": {
        	src: "../tools/config/breakpoints-config.json",
        	dest: ".tmp/breakpoints-config.json"
        }
    };
};