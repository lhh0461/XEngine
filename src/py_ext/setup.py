from distutils.core import setup, Extension
import os
import platform

path = os.path.realpath(__file__)
cur_dir = os.path.dirname(path)
#extra_compile_args = ["-pg"]
#extra_compile_args = ["-DDIRTY_DUMP_DEBUG", "-DDIRTY_MAP_CHECK"]
extra_compile_args = []

module_pydirty = Extension('dirty',
                    extra_compile_args = extra_compile_args,
                    include_dirs  = [os.path.join(cur_dir, '..')],
                    library_dirs = [],
                    libraries = [],
                    sources = [
                        os.path.join(cur_dir, 'pydirty.c'),
                        os.path.join(cur_dir, 'pydirty_dict.c'),
                        os.path.join(cur_dir, 'dirty.c'),
                    ],
                    #language = "c++",
)

setup (name = 'dirty',
       version = '1.0',
       description = 'This is a DirtyDict',
       author = 'jun',
       author_email = 'lzj0051@175game.com',
       ext_modules = [module_pydirty])

