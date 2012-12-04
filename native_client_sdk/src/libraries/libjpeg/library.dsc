{
  'TOOLS': ['newlib', 'glibc', 'linux', 'win'],
  'SEARCH': [
    '../../../../third_party/libjpeg',
  ],
  'TARGETS': [
    {
      'NAME' : 'libjpeg',
      'TYPE' : 'lib',
      'INCLUDES': ['../../include/libjpeg'],      
      'SOURCES' : [
        'jcapimin.c',
        'jcapistd.c',
        'jccoefct.c',
        'jccolor.c',
        'jcdctmgr.c',
        'jchuff.c',
        'jcinit.c',
        'jcmainct.c',
        'jcmarker.c',
        'jcmaster.c',
        'jcomapi.c',
        'jcparam.c',
        'jcphuff.c',
        'jcprepct.c',
        'jcsample.c',
        'jdapimin.c',
        'jdapistd.c',
        'jdatadst.c',
        'jdatasrc.c',
        'jdcoefct.c',
        'jdcolor.c',
        'jddctmgr.c',
        'jdhuff.c',
        'jdinput.c',
        'jdmainct.c',
        'jdmarker.c',
        'jdmaster.c',
        'jdmerge.c',
        'jdphuff.c',
        'jdpostct.c',
        'jdsample.c',
        'jerror.c',
        'jfdctflt.c',
        'jfdctfst.c',
        'jfdctint.c',
        'jidctflt.c',
        'jidctfst.c',
        'jidctint.c',
        'jidctred.c',
        'jmemmgr.c',
        'jmemnobs.c',
        'jquant1.c',
        'jquant2.c',
        'jutils.c',
      ],
    }
  ],
  'HEADERS': [
    {
      'DEST': 'include/libjpeg',
      'FILES': [
        'jchuff.h',
        'jconfig.h',
        'jdct.h',
        'jdhuff.h',
        'jerror.h',
        'jinclude.h',
        'jmemsys.h',
        'jmorecfg.h',
        'jpegint.h',
        'jpeglib.h',
        'jpeglibmangler.h',
        'jversion.h',
      ],
    }
  ],
  'DATA': [
    'LICENSE',
    'README',
    'README.chromium',
  ],
  'DEST': 'src',
  'NAME': 'libjpeg',
  'EXPERIMENTAL': True,
}

