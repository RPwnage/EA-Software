import pyscreenshot as ImageGrab
import StringIO
import web

"""
Module for Screen Capture operation interface
"""

urls = (
    '/fullscreen', 'FullScreen'
)


class FullScreen:
    ## GET method to capture full screen in png.
    #
    # @result captured png image
    def GET(self):
        try:
            im = ImageGrab.grab()
            mem_file = StringIO.StringIO()
            im.save(mem_file, 'png')
            web.header('Content-Type', 'image/png')
            return mem_file.getvalue()
        except Exception as e:
            print(e)
            return web.internalerror()


screencaptureop = web.application(urls, locals())