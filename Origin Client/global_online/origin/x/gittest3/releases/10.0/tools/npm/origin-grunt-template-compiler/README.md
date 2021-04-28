> Origin Template Compiler Grunt Task



## Getting Started
This plugin requires Grunt `~0.4.0`

If you haven't used [Grunt](http://gruntjs.com/) before, be sure to check out the [Getting Started](http://gruntjs.com/getting-started) guide, as it explains how to create a [Gruntfile](http://gruntjs.com/sample-gruntfile) as well as install and use Grunt plugins. Once you're familiar with that process, you may install this plugin with this command:

Since this is a private library for use with origin build tools, in this project folder type:
```shell
npm link
```
Wherever you'd like to use this package, type

```shell
npm link origin-grunt-template-compiler
```

Once the plugin has been installed, it may be enabled inside your Gruntfile with this line of JavaScript:

```js
grunt.loadNpmTasks('origin-grunt-template-compiler');
```



### Example config

```javascript
grunt.initConfig({
    templatecompile: {                                      //Task
        options: {                                          //Task Options
            retainTemplateReference: true,
            removeComments: true,
            collapseWhitespace: true,
            paths: {                                        //File references
                '@components_src': 'test/fixtures',
                '@app_src': 'test/fixtures'
            }
        },
        compile: {                                          //Task Files
            files: [{
                cwd: 'test',
                dest: 'tmp',
                src: [
                    'fixtures/directives/scripts/**/*.js'
                ]
            }]
        }
    },
});

Now add this to any defined task in the grunt.registerTask group eg.

grunt.registerTask('build', ['...', 'templatecompile', '...'])
```



### Directive File Usage

To use this compiler, you will need to annotate your templateUrl paths with a special prefix that allows the compiler to find your html files. The correct AngularJS
directive pattern will look something like this when used with the compiler:

```javascript
.directive('myCustomer', function() {
  return {
    templateUrl: '@user_ui_bundle/example/database/views/my-customer.html'
  };
});
```

@user_ui_bundle is a base path reference that tells grunt where to find the HTML files for inclusion. The naming strategy for this prefix will be influenced by the
project bundling/layout. You may define as many prefixes as you like in the required paths option.


### Options

This task delegates minification functions to [Htmlmin](https://github.com/gruntjs/grunt-contrib-htmlmin), so please condiser the [Htmlmin documentation](https://github.com/gruntjs/grunt-contrib-htmlmin) for advanced configuration

### paths
Type: `Object`
Inclusion: Required

This setting contains a dictionary {bundle_name: realpath} the bundle naming convention is agreed upon in the development of directives. eg '@component_src' is an alias to src/directives in the project. These pairs will tell the compiler where the html files can be found on the webserver to either reference/copy or include as a string in the template. The path is relative to the CWD param of the file undergoing optimization.
