from distutils.core import setup, Extension

pulsemodule = Extension('pulsesimple',
                    libraries = ['pulse-simple'],
                    library_dirs = ['/usr/lib'],
                    sources = ['pulsesimple.c'])

setup (name = 'pulsesimple',
       version = '0.1',
       description = 'Python binding for the Pulseadudio simple API',
       author = 'Damir Jelić',
       author_email = 'poljarinho@gmail.com',
       ext_modules = [pulsemodule])
