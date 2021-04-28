> Origin Font Compiler Grunt Task



## Getting Started
This plugin requires Grunt `~0.4.0`

If you haven't used [Grunt](http://gruntjs.com/) before, be sure to check out the [Getting Started](http://gruntjs.com/getting-started) guide, as it explains how to create a [Gruntfile](http://gruntjs.com/sample-gruntfile) as well as install and use Grunt plugins. Once you're familiar with that process, you may install this plugin with this command:

Since this is a private library for use with origin build tools, in this project folder type:
```shell
npm link
```
Wherever you'd like to use this package, type

```shell
npm link origin-grunt-font-compiler
```

Once the plugin has been installed, it may be enabled inside your Gruntfile with this line of JavaScript:

```js
grunt.loadNpmTasks('origin-grunt-font-compiler');
```



### Example Config

```javascript
grunt.initConfig({
    fontcompile: {
        compile: {
            files: [{
                cwd: 'test',
                dest: 'tmp',
                src: [
                    'fixtures/less/fonts.less'
                ]
            }]
        }
    },
});

Now add this to any defined task in the grunt.registerTask group eg.

grunt.registerTask('build', ['...', 'fontcompile', '...'])
```



### Creating a LESS File

The compiler will scan the input .LESS file for font path variables prefixed with @font-file-*. font file paths are relative
to the location of the LESS file:

project/less/fonts.less
```less
@font-file-origin-glyph-icons-woff: "../fonts/origin.woff";

@font-face {
    font-family: 'origin';
    src: url(@font-file-origin-glyph-icons-woff) format('woff');
    font-weight: normal;
    font-style: normal;
}
```

after compilation it will look like this:

temp/less/fonts.less
```less
@font-file-origin-glyph-icons-woff: "data:application/x-font-woff;charset=utf-8;base64,d09....";

@font-face {
    font-family: 'origin';
    src: url(@font-file-origin-glyph-icons-woff) format('woff');
    font-weight: normal;
    font-style: normal;
}
```
The fontcompile task replaces the file path with a valid data uri and writes the modified less to a staging area for further less compilation/minification steps.
Have a look at the test fixtures for precise usage and further visual testing tools. **This step must be run before the less grunt task**.