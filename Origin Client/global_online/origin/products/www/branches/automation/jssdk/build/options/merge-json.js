module.exports = function() {

    return {
        "config": {
            src: ["../tools/config/jssdk-origin-config.json", "../tools/config/environment/production/jssdk-nonorigin-config.json"],
            dest: "tmp/jssdk-config.json"
        }
    };
};