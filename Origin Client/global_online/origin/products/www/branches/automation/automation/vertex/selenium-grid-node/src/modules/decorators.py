import functools
import web

def perform_call():
    def decorator(func):
        @functools.wraps(func)
        def wrapper(*args, **vars):
            try:
                func(*args, **vars)
            except:
                return web.badrequest()
        return wrapper
    return decorator