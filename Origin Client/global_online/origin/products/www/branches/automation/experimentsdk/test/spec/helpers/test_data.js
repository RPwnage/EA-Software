var segmentData = {
      "all-origin-access-users": {
        "segmentName": "all-origin-access-users",
        "rule": {
          "operator": "AND",
          "parameters": {
            "locales": {
              "arg": [
                "en-us"
              ]
            },
            "storeFronts": {
              "arg": [
                "usa"
              ]
            },
            "originAccessUser": {
              "arg": true
            }
          }
        }
      },
      "na-origin-access-users": {
        "segmentName": "na-origin-access-users",
        "rule": {
          "operator": "AND",
          "parameters": {
            "storeFronts": {
              "arg": [
                "can",
                "usa"
              ]
            },
            "originAccessUser": {
              "arg": true
            }
          }
        }
      },
      "all-storefront-except-nzl": {
        "segmentName": "all-storefront-except-nzl",
        "rule": {
          "operator": "AND",
          "parameters": {
            "storeFronts": {
              "arg": [
                "aus",
                "bel",
                "bra",
                "can",
                "deu",
                "dnk",
                "esp",
                "fin",
                "fra",
                "gbr",
                "hkg",
                "ind",
                "irl",
                "ita",
                "jpn",
                "kor",
                "mex",
                "nld",
                "nor",
                "pol",
                "rus",
                "sgp",
                "swe",
                "tha",
                "twn",
                "usa",
                "zaf"
              ]
            },
            "originAccessUser": {
              "arg": false
            }
          }
        }

      }
    },

    experimentData = {
        "origin-access-impulse-buying---black-friday-2016": {
            "experimentId": "25334a0c-1370-4d70-977b-1869132c8273",
            "experimentName": "origin-access-impulse-buying---black-friday-2016",
            "segments": [
                "na-origin-access-users"
            ],
            "startDate": "2016-11-29T17:28:00.000-08:00",
            "endDate": "2099-12-07T17:28:00.000-08:00",
            "variantOverride": "B",
            "variants": [{
                "name": "Control",
                "percentage": 50
            },{
                "name": "A",
                "percentage": 25
            },{
                "name": "B",
                "percentage": 25
            }]
        },
        "my-home-video-button": {
            "experimentId": "f68d6ddf-06e9-463d-aae1-2d7df70d92b6",
            "experimentName": "my-home-video-button",
            "segments": [
                "all-storefront-except-nzl"
            ],
            "startDate": "2016-11-30T08:29:00.000-08:00",
            "endDate": "2099-12-31T08:29:00.000-08:00",
            "variantOverride": "",
            "variants": [{
                "name": "control",
                "percentage": 50
            },{
                "name": "showButton",
                "percentage": 50
            }
            ]
        },
        "experiment1": {
            "experimentId": "39ea272a-521b-4d80-a23c-9889f0829d09",
            "experimentName": "experiment1",
            "segments": [
            ],
            "startDate": "2016-12-03T00:22:00.000-08:00",
            "endDate": "2099-12-31T00:22:00.000-08:00",
            "variantOverride": "",
            "variants": [{
                "name": "control",
                "percentage": 60
            },{
                "name": "variantA",
                "percentage": 20
            },{
                "name": "variantB",
                "percentage": 20
            }
            ]
        }
    },

    segmentDataStr = JSON.stringify(segmentData),
    experimentDataStr = JSON.stringify(experimentData);

var testSegmentResponse = {
      success: {
        status: 200,
        responseText: segmentDataStr
      }
    },

    testExperimentResponse = {
        success: {
            status: 200,
            responseText: experimentDataStr
        }
    };