// Run some tasks in parallel to speed up the build process
module.exports = function() {
    return {
        server: [
            'copy:fonts',
        ],
        test: [
            'copy:fonts',
        ],
        dist: [
            'copy:fonts',
            'copy:images'
        ]
    };
};