from distutils.core import setup, Extension
setup(name="noddy", version="1.0",
      ext_modules=[Extension("save", ["py_save_type.c"])])
