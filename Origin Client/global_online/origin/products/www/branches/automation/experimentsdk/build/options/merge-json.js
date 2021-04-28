module.exports = function() {

    return {
        "config": {
            src: ["./tools/config/experimentsdk-config.json", "./tools/config/environment/production/experimentsdk-noneax-config.json"],
            dest: "tmp/experimentsdk-config.json"
        }
    };
};