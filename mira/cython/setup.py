from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize

import os.path

setup(ext_modules = cythonize("svector.pyx", language="c++"))
setup(ext_modules = cythonize(Extension("cost", ["cost.pyx", "logp.c"]), language="c++"))
setup(ext_modules = cythonize("sym.pyx", language="c++"))
setup(ext_modules = cythonize(Extension("rule", ["rule.pyx", "strutil.c"]), language="c++"))

nnlm_root = '/home/nlg-01/chiangd/nnlm'
setup(ext_modules=cythonize(
        [Extension("lm_nplm",
                   sources=["lm_nplm.pyx"],
                   language="c++",
                   include_dirs=[os.path.join(nnlm_root, 'src'),
                                 os.path.join(nnlm_root, '3rdparty/tclap/include'),
                                 os.path.join(nnlm_root, '3rdparty/eigen'),
                                 '/usr/usc/boost/1.51.0/include',
                                 ],
                   extra_compile_args=[#'-fopenmp', 
                                       '-O3'],
                   )],
        include_path=[os.path.join(nnlm_root, 'src/python')]
        ))
