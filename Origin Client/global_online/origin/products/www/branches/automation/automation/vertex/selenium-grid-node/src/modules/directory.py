import json
import os
import shutil
import web
import glob

"""
Module for Directory operation interface
"""

urls = (
    '/create', 'Create',
    '/delete', 'Delete',
    '/search', 'Search',
	'/copy', 'Copy',
    '/list', 'List',
	'/size', 'Size',
    '/exist', 'Exist'
)


def getFolderSize(path):
	totalSize = 0
	if os.path.islink(path):
		pass
	elif os.path.isdir(path):
		sub_items = glob.glob(path + '\\*')
		for sub_item in sub_items:
			totalSize += getFolderSize(sub_item)
	else:
		totalSize += os.path.getsize(path)
	return totalSize


def make_dir(path):
    if not os.path.isdir(path):
        os.mkdir(path)


def recursive_copy(src, dst):
	if os.path.islink(src):
		linkto = os.readlink(src)
		os.symlink(linkto, dst)
	elif os.path.isdir(src):
		make_dir(dst)
		sub_items = glob.glob(src + '\\*')
		for sub_item in sub_items:
			recursive_copy(sub_item, dst + '\\' + sub_item.split('\\')[-1])
	else:
		shutil.copy(src, dst)


class Create:
    ## GET method to create a new directory if not exists.
    #
    # @param path (mandatory) -- path of new directory to be created. Example: <URL>?path=c:\sandbox\create
    # @result 'ok' if succeed
    def GET(self):
        try:
            path = web.input().path
            if not os.path.isdir(path):
                try:
                    os.makedirs(path)
                except Exception as e:
                    print(e)
                    return web.internalerror()
            return 'ok'
        except:
            return web.badrequest()


class Delete:
    ## GET method to delete a directory and all its content.
    #
    # @param path (mandatory) -- path of directory to be deleted. Example: <URL>?path=c:\sandbox\tobedelete
    # @result 'ok' if succeed
    def GET(self):
        try:
            path = web.input().path
            if os.path.isdir(path):
                try:
                    shutil.rmtree(path, ignore_errors=True)
                except Exception as e:
                    print(e)
                    return web.internalerror()
            return 'ok'
        except:
            return web.badrequest()


class Search:
    ## POST method to find files which its filename contains all given strings under given directory.
    #
    # @param path (mandatory) -- path of directory to be searched. Example: <URL>?path=c:\sandbox\dir
    # @param POST message body -- JSON Array contains search keyword strings. Example: ['win', 'chrome']
    # @result JSON Array contains filename
    def POST(self):
        try:
            result = set()
            path = web.input().path
            if os.path.isdir(path):
                try:
                    matches = json.loads(unicode(web.data()))
                    # list file only
                    names = (file for file in os.listdir(path) if os.path.isfile(os.path.join(path, file)))
                    for name in names:
                        matched = True if len(matches) > 0 else False
                        for match in matches:
                            if match not in name:
                                matched = False
                                break
                        if matched:
                            result.add(name)
                except Exception as e:
                    print(e)
                    return web.internalerror()
            return json.dumps(list(result))
        except:
            return web.badrequest()

			
class Copy:
    ## GET method to copy a directory and all its content.
    #
    # @param source (mandatory) -- path of original dicectory. Example: <URL>?source=c:\sandbox\sourceDir
    # @param dest (mandatory) -- path of new directory location. Example: <URL>?dest=c:\sandbox\destDir
    # @result 'ok' if succeed
    def GET(self):
        try:
            source = web.input().source
            if not os.path.isdir(source):
                return web.badrequest();
            dest = web.input().dest
            try:
				recursive_copy(source, dest + '\\' + source.split('\\')[-1])
				return 'ok'
            except Exception as e:
				print(e)
				return web.internalerror()
        except:
            return web.badrequest()


class List:
    ## GET method to get a list of file/directory under given directory.
    #
    # @param path (mandatory) -- path of directory to be listed. Example: <URL>?path=c:\sandbox\dir
    # @result JSON Array contains filename
    def GET(self):
        try:
            path = web.input().path
            if os.path.isdir(path):
                try:
                    return json.dumps(os.listdir(path))
                except Exception as e:
                    print(e)
                    return web.internalerror()
            return '[]'
        except:
            return web.badrequest()


class Size:
    ## GET method to retrive dicectory length in bytes.
    #
    # @param path (mandatory) -- path of directory to be checked. Example: <URL>?path=c:\sandbox\dir
    # @result size in byte
    def GET(self):
        try:
            path = web.input().path
            if not os.path.isdir(path):
                return web.badrequest()
            try:
				return getFolderSize(path)
            except Exception as e:
                print(e)
                return web.internalerror()
        except:
            return web.badrequest();

			
class Exist:
    ## GET method to check if a directory exists.
    #
    # @param path (mandatory) -- path of directory to be checked. Example: <URL>?path=c:\sandbox\dir
    # @result True if directory exists or False otherwise
    def GET(self):
        try:
            path = web.input().path
            return os.path.isdir(path)
        except:
            return web.badrequest()


directoryop = web.application(urls, locals())