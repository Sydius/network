VERSION='1.0.0'
APPNAME='networking'

srcdir='.'
blddir='build'

def options(opt):
    opt.load('compiler_cxx')
    opt.add_option('--pantheios', action='store', help='Location of Pantheios library')
    opt.add_option('--stlsoft', action='store', help='Location of STLSOFT')

def configure(conf):
    conf.load('compiler_cxx')
    conf.env.PANTHEIOS = conf.options.pantheios
    conf.env.STLSOFT = conf.options.stlsoft

def build(bld):
    bld.program(
        source=['main.cpp', 'shared.cpp', 'connection.cpp'],
        includes=['../call-with-tuple', '../serialize-tuple', '../dynamic-invocation',
            bld.env.PANTHEIOS + '/include',
            bld.env.STLSOFT + '/include',
            ],
        libpath=[bld.env.PANTHEIOS + '/lib'],
        target='game',
        lib=['boost_system-mt', 'boost_serialization',
            'pantheios.1.core.gcc45.file64bit.mt',
            'pantheios.1.be.fprintf.gcc45.file64bit.mt',
            'pantheios.1.bec.fprintf.gcc45.file64bit.mt',
            'pantheios.1.fe.all.gcc45.file64bit.mt',
            'pantheios.1.util.gcc45.file64bit.mt',
            ],
        cxxflags='-O3 --std=c++0x --pedantic -Wall -Wfatal-errors -DUSE_PANTHEIOS -Weffc++ -fdiagnostics-show-option'
    )
