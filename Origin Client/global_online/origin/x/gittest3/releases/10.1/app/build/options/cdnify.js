// Replace Google CDN references

module.exports = function() {
    return {
        dist: {
            html: ['<%= yeoman.dist %>/*.html']
        }
    };
};