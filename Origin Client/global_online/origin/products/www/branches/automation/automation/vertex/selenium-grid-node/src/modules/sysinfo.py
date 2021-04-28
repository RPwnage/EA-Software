import json
import os
import web

"""
Module for System Information operation interface
"""

urls = (
    '/environ', 'Environ'
)


class Environ:
    ## GET method to retrieve agent local system environment variables.
    #
    # @result data in JSON format
    def GET(self):
        try:
            return json.dumps(os.environ.__dict__, ensure_ascii=False)
        except Exception as e:
            print(e)
            return web.internalerror()


sysinfoop = web.application(urls, locals())