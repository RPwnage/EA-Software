# Framework Documentation Overview #

The Framework documentation toolchain used to generate the static website is called [Hugo](https://gohugo.io).

The documentation itself is written in [Markdown](https://en.wikipedia.org/wiki/Markdown), combined with some embedded HTML and Hugo specific markup.


# Working on documentation #

To work on documentation there are a couple of options.


For simple changes, you can simply edit the markdown file directly, preview it in a [Markdown viewer/editor](https://www.shopify.com/partners/blog/10-of-the-best-markdown-editors). I use Visual Studio Code, but any will do.

When adding new pages or doing larger changes, it is recommended you live edit the documentation locally by going to the documentation folder located in:
`//fbstream/dev-na-cm/TnT/Build/Framework/source/hugodoc/`
and launch the hugo application using:
``` 
> bin\hugo.exe serve
```

This will launch a local webserver at the location mentioned in the output which monitors changes and rebuilds the documentation live.

Once you're happy with the changes, check in and the CI will automatically deploy the latest changes.

## Hugo Configuration ##

The main [configuration file](https://gohugo.io/getting-started/configuration/) can be found in the root folder and is named `config.toml` and contains the configuration for both Hugo and the theme.

## Content Layout ##

The main documentation lives in the `content` folder and has the following layout:
```
content
├── level-one
│   ├── level-two
│   │   ├── level-three
│   │   │   ├── level-four
│   │   │   │   ├── _index.md
│   │   │   │   ├── page-4-a.md
│   │   │   │   ├── page-4-b.md
│   │   │   │   |── page-4-c.md
|   |   |   |   └── page-4-c
|   |   |   |       └── image.png
│   │   │   ├── _index.md
│   │   │   ├── page-3-a.md
│   │   │   ├── page-3-b.md
│   │   │   └── page-3-c.md
│   │   ├── _index.md
│   │   ├── page-2-a.md
│   │   ├── page-2-b.md
│   │   └── page-2-c.md
│   ├── _index.md
│   ├── page-1-a.md
│   ├── page-1-b.md
│   └── page-1-c.md
├── _index.md
└── page-top.md
```

The naming of folders and names are important as the generation of the documentation on a linux platform. Therefore, it is best to stick to **lowercase with no spaces or special characters**.

## Theme ##

The Framework documentation uses the [DocDock theme](https://docdock.netlify.com/) as its base, with some customizations.

The theme is located in `themes\docdock`.

The theme also adds some extra functionality that is documented on the [theme's website](https://docdock.netlify.com/).

Customizations for the Framework documentation include:
* CSS overrides: `static\css\overrides.css`
* Extra Shortcodes: `layout\shortcodes`

## Shortcodes ##

Shortcodes are extra tags in the markdown documentation that allow for custom scripting behavior.

## Hugo and theme shortcodes ##

Some interesting shortcodes you can use are:

* [highlight](https://gohugo.io/content-management/syntax-highlighting/): Highlight sections of code and/or text.
* [ref and relref](https://gohugo.io/content-management/shortcodes/#ref-and-relref): return permalink or relative permalink to page
* [alert](https://docdock.netlify.com/shortcodes/alert/) [panel](https://docdock.netlify.com/shortcodes/panel/) and [notice](https://docdock.netlify.com/shortcodes/notice/): Highlight information by putting it in a box.
* [expand](https://docdock.netlify.com/shortcodes/expand/): Expandable text.
* [icon](https://docdock.netlify.com/shortcodes/icon/): Add an icon to your page.
* [mermaid](https://docdock.netlify.com/shortcodes/mermaid/): Render a flowchart
* [revealjs](https://docdock.netlify.com/shortcodes/revealjs/): Render markdown presentation with revealjs.
* [children](https://docdock.netlify.com/shortcodes/children/): List the children of the current page

More shortcodes can be found on the [hugo documentation](https://gohugo.io/content-management/shortcodes/) and the [theme documentation](https://docdock.netlify.com/shortcodes).
Or you can write your own! 

### Custom Framework shortcodes ###

The `data` folder is used to store data related to custom functionality, mainly related with [Hugo Shortcodes](https://gohugo.io/content-management/shortcodes/). 

At this point, there are a couple of custom shortcodes:
### **Task Shortcode** ###
Shortcode translation from C# Framework Task references to their documentation.

#### Example: #### 
```md
Structured Xml layer introduces {{< task "EA.Eaconfig.Structured.BuildTypeTask" >}} task that makes custom buildtype definition simpler. 
```

#### Implementation ####

The implementation for this shortcode can be found in `layout\shortcodes\task.html`. The data the shortcode references can be found in `data\tasks.toml`


### **include Shortcode** ###
Include an external file as an excerpt. Used for code examples, etc.

#### Example: #### 
```xml
{{%include filename="/reference/packages/manifest.xml-file/_index/manifestintroexample.xml" /%}}

```

#### Implementation ####

The implementation for this shortcode can be found in `layout\shortcodes\include.html`.

### **url Shortcode** ###
Shortcode translation from various named links to their actual urls.

#### Example: #### 
```md
Framework can be downloaded from the{{< url PackageServer >}} or sync&#39;ed from our {{< url PerforceDepot >}}on the Frostbite Perforce server in //packages/Framework.
It is also included with Frostbite in TnT/Bin/Framework.
```

#### Implementation ####

The implementation for this shortcode can be found in `layout\shortcodes\url.html`. The data the shortcode references can be found in `data\urls.toml`.

# Documentation deployment process #

Currently the [Framework8.BuildTestSubmit](https://eac-freeze3.eac.ad.ea.com/job/dev-na-cm/job/Framework8.BuildTestSubmit/) job is used to generate the documentation and does the following:

* Get latest perforce documentation from: 

        //fbstream/dev-na-cm/TnT/Build/Framework/source/hugodoc/

* Generate C# documentation using `FwDocsToMd` project in the Framework solution
* Merge and submit to Perforce
* Mirror to the [FwDocs Gitlab repository](https://gitlab.ea.com/frostbite-ew/framework/fwdocs)
* Deploy to Gitlab Pages at https://frostbite-ew.gitlab.ea.com/framework/fwdocs