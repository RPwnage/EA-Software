module.exports = function() {
    return {
        "config": {
            src: ["../tools/config/telemetry/*.json"],
            dest: ".tmp/telemetry-config.json"
        }
    };
};