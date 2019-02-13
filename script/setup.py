from distutils.core import setup, Extension
import os
import platform

path = os.path.realpath(__file__)
cur_dir = os.path.dirname(path)
extra_compile_args = ["-g", "-Wall", "-O0"]
#extra_compile_args = ["-DDIRTY_DUMP_DEBUG", "-DDIRTY_MAP_CHECK"]
#extra_compile_args = []

module_pydirty = Extension('dirty',
                    extra_compile_args = extra_compile_args,
                    include_dirs  = [
                        os.path.join(cur_dir, '..'), 
                        os.path.join(cur_dir, '../../include'), 
                    ],
                    library_dirs = [],
                    libraries = [],
                    sources = [
                        os.path.join(cur_dir, 'pydirty.c'),
                        os.path.join(cur_dir, 'pydirty_dict.c'),
                        os.path.join(cur_dir, 'pydirty_list.c'),
                        os.path.join(cur_dir, 'dirty.c'),
                        os.path.join('../mem_pool.c'),
                        os.path.join('../log.c'),
                    ],
                    #language = "c++",
)

module_timer = Extension('dirty',
                    extra_compile_args = extra_compile_args,
                    include_dirs  = [
                        os.path.join(cur_dir, '..'), 
                        os.path.join(cur_dir, '../../include'), 
                    ],
                    library_dirs = [],
                    libraries = [],
                    sources = [
                        os.path.join(cur_dir, 'pydirty.c'),
                        os.path.join(cur_dir, 'pydirty_dict.c'),
                        os.path.join(cur_dir, 'pydirty_list.c'),
                        os.path.join(cur_dir, 'dirty.c'),
                        os.path.join('../mem_pool.c'),
                        os.path.join('../log.c'),
                    ],
                    #language = "c++",
)

setup (name = 'dirty',
       version = '1.0',
       description = 'This is a DirtyDict',
       author = 'jun',
       author_email = 'lzj0051@175game.com',
       ext_modules = [module_pydirty])

setup (name = 'timer',
       version = '1.0',
       description = 'This is a DirtyDict',
       author = 'jun',
       author_email = 'lzj0051@175game.com',
       ext_modules = [module_timer])
