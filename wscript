VERSION='1.0.0'
APPNAME='networking'

srcdir='.'
blddir='build'

def options(opt):
    opt.load('compiler_cxx')

def configure(conf):
    conf.load('compiler_cxx')

def build(bld):
    bld.program(
        source=['main.cpp', 'shared.cpp'],
        includes=['../call-with-tuple', '../serialize-tuple', '../dynamic-invocation'],
        target='game',
        lib=['boost_system-mt', 'boost_serialization'],
        cxxflags='-O3 --std=c++0x --pedantic -Wall'
    )
