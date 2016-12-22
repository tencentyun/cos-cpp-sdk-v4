cc_library(
    name = 'cossdk',
    srcs = [
        'CosApi.cpp',
        'CosConfig.cpp',
        'CosSysConfig.cpp',
		'Auth.cpp',
	],
    deps = [
	   '//src/util:util'
	   '//src/thread:thread'
	   '//src/request:request'
	   '//src/op:op'
	   '//toft/system/threading:threading'
    ],
    incs = [
	    '//include/'
		'//include/curl'
		'//include/json'
		'//include/l5'
		'//include/op'
		'//include/request'
		'//include/thread'
		'//include/util'
		'//toft/system/threading'
    ],
    extra_cppflags = [
        '-pthread',
        '-fPIC',
        '-D_REENTRANT',
        '-D_GNU_SOURCE',
        '-Wno-sign-compare',
        '-fno-strict-aliasing',
    ]
)
