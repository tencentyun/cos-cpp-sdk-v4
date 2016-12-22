cc_library(
    name = 'op',
    srcs = [
        'BaseOp.cpp',
        'FileOp.cpp',
        'FileUploadTask.cpp',
        'FolderOp.cpp',
    ],
    deps = [
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
