// Performs rewrites based on filerev and the useminPrepare configuration
module.exports = function() {
    return {
        html: ['<%= yeoman.dist %>/{,*/}*.html'],
        css: ['<%= yeoman.dist %>/styles/{,*/}*.css'],

        options: {
            html: 'index.html',
            assetsDirs: ['<%= yeoman.dist %>', '<%= yeoman.dist %>/images', '<%= yeoman.dist %>/scripts'],
            patterns: {
                html: [
                    [/new Worker\(['"]([^"']+)['"]/gm,
                        'Update the Workers to reference our concat/min/revved script files'
                    ]
                ]
            },
            blockReplacements: {
                worker: function(block) {
                    return 'new Worker("' + block.dest + '");';
                }
            }
        }
    };
};