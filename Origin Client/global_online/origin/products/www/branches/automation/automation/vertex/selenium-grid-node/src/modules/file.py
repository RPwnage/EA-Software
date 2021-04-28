import codecs
import os
import shutil
import sys
import web

"""
Module for File operation interface
"""

urls = (
    '/create', 'Create',
    '/delete', 'Delete',
    '/read', 'Read',
    '/write', 'Write',
    '/copy', 'Copy',
    '/move', 'Move',
    '/size', 'Size',
    '/exist', 'Exist'
)


class Create:
    ## GET method to create a new file.
    #
    # @param path (mandatory) -- path of file to be created. Example: <URL>?path=c:\sandbox\newfile.txt
    # @param size (optional) -- file initial size in byte. Example: <URL>?size=1024
    # @result 'ok' if succeed
    def GET(self):
        try:
            path = web.input().path
            try:
                with open(path, 'w+') as f:
                    try:
                        size = int(web.input().size)
                    except:
                        pass
                    else:
                        f.truncate(size)
                    f.close()
                    return 'ok'
            except Exception as e:
                print(e)
                return web.internalerror()
        except:
            return web.badrequest()


class Delete:
    ## GET method to delete a file.
    #
    # @param path (mandatory) -- path of file to be deleted. Example: <URL>?path=c:\sandbox\tobedelete.txt
    # @result 'ok' if succeed
    def GET(self):
        try:
            path = web.input().path
            if os.path.isfile(path):
                try:
                    os.remove(path)
                except Exception as e:
                    print(e)
                    return web.internalerror()
            return 'ok'
        except:
            return web.badrequest()


class Read:
    ## GET method to read file content.
    #
    # @param path (mandatory) -- path of file to be read. Example: <URL>?path=c:\sandbox\target.txt
    # @result file content
    def GET(self):
        try:
            path = web.input().path
            if not os.path.isfile(path):
                return web.badrequest();
            try:
                with codecs.open(path, 'r', encoding='utf-8') as f:
                    file_text = f.read()
                    f.close()
                    return file_text
            except Exception as e:
                print(e)
                return web.internalerror()
        except:
            return web.badrequest()


class Write:
    ## POST method to write data to a file, creates a new file is not exists.
    #
    # @param path (mandatory) -- path of file to be written. Example: <URL>?path=c:\sandbox\target.txt
    # @param POST message body -- Data to be written.
    # @result 'ok' if succeed
    def POST(self):
        try:
            path = web.input().path
            try:
                with codecs.open(path, 'w+', encoding='utf-8') as f:
                    f.write(unicode(web.data(), 'utf8'))
                    f.close()
                return 'ok'
            except Exception as e:
                print(e)
                return web.internalerror()
        except:
            return web.badrequest()


class Copy:
    ## GET method to copy a file.
    #
    # @param source (mandatory) -- path of original file. Example: <URL>?source=c:\sandbox\source.txt
    # @param dest (mandatory) -- path of new file location. Example: <URL>?dest=c:\sandbox\dest.txt
    # @result 'ok' if succeed
    def GET(self):
        try:
            source = web.input().source
            if not os.path.isfile(source):
                return web.badrequest();
            dest = web.input().dest
            try:
                shutil.copyfile(source, dest)
                return 'ok'
            except Exception as e:
                print(e)
                return web.internalerror()
        except:
            return web.badrequest()


class Move:
    ## GET method to move a file.
    #
    # @param source (mandatory) -- path of original file. Example: <URL>?source=c:\sandbox\source.txt
    # @param dest (mandatory) -- path of new file location. Example: <URL>?dest=c:\sandbox\dest.txt
    # @result 'ok' if succeed
    def GET(self):
        try:
            source = web.input().source
            if not os.path.isfile(source):
                return web.badrequest()
            dest = web.input().dest
            try:
                shutil.move(source, dest)
                return 'ok'
            except Exception as e:
                print(e)
                return web.internalerror()
        except:
            return web.badrequest();


class Size:
    ## GET method to retrive file length in bytes.
    #
    # @param path (mandatory) -- path of file to be checked. Example: <URL>?path=c:\sandbox\target.txt
    # @result size in byte
    def GET(self):
        try:
            path = web.input().path
            if not os.path.isfile(path):
                return web.badrequest()
            try:
                statinfo = os.stat(path)
                return statinfo.st_size
            except Exception as e:
                print(e)
                return web.internalerror()
        except:
            return web.badrequest();


class Exist:
    ## GET method to check if a file exists.
    #
    # @param path (mandatory) -- path of file to be checked. Example: <URL>?path=c:\sandbox\target.txt
    # @result True if file exists or False otherwise
    def GET(self):
        try:
            path = web.input().path
            return os.path.isfile(path)
        except:
            return web.badrequest()


fileop = web.application(urls, locals())