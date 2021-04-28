
    var ShowSocialMediaEnumeration = {
        "true": "true",
        "false": "false"
    };

    var ButtonTypeEnumeration = {
        "primary": "primary",
        "secondary": "secondary",
        "round": "round",
        "transparent": "transparent"
    };

    var SimpleButtonTypeEnumeration = {
      "primary": "primary",
      "secondary": "secondary"
    };

    var ExtenedButtonTypeEnumeration = {
        "primary": "primary",
        "secondary": "secondary",
        "round": "round",
        "transparent": "transparent"
      };

      var OptionalButtonTypeEnumeration = {
        "none": "",
        "primary": "primary",
        "secondary": "secondary",
        "transparent": "transparent"
      };

      var OptionalButtonTypeEnumeration = {
        "secondary": "secondary",
        "transparent": "transparent",
        "round": "round"
      };

    /**@ngdoc directive
      * @param {TextSlideEnumeration} slide textslide boolean
      * @param {aSlideEnumeration} slide a textslide boolean
      * @param {ButtonTypeEnumeration} button
    **/

    function bannedEnumeration() {
        console.log(ShowSocialMediaEnumeration);
        console.log(ButtonTypeEnumeration);
        console.log(SimpleButtonTypeEnumeration);
        console.log(ExtenedButtonTypeEnumeration);
        console.log(OptionalButtonTypeEnumeration);
        console.log(OptionalButtonTypeEnumeration);
    }

    bannedEnumeration();
