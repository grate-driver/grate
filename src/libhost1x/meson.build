libhost1x_sources =  files(
	'dri-display.c',
	'host1x.c',
	'host1x-drm.c',
	'host1x-dummy.c',
	'host1x-framebuffer.c',
	'host1x-gr2d.c',
	'host1x-gr3d.c',
	'host1x-nvhost.c',
	'host1x-pixelbuffer.c',
	'host1x-private.h',
	'nvhost.c',
	'nvhost-display.c',
	'nvhost-gr2d.c',
	'nvhost-gr3d.c',
	'nvhost.h',
	'nvhost-nvmap.c',
	'nvhost-nvmap.h',
	'nvhost-nvmap.h',
	'tegra_dc_ext.h',
	'tegra_drm.h',
	'x11-display.c',
	'x11-display.h'
)

libhost1x_c_args = []
libhost1x_deps = [libdrm, libpng]

if x11.found() and \
   dependency('xcb', required : false).found() and \
   dependency('xcb-image', required : false).found() and \
   dependency('xcb-dri2', required : false).found() and \
   dependency('xcb-icccm', required : false).found()
	libhost1x_c_args += '-DHAVE_XCB'
	libhost1x_deps += dependency('xcb')
	libhost1x_deps += dependency('xcb-image')
	libhost1x_deps += dependency('xcb-dri2')
	libhost1x_deps += dependency('xcb-icccm')
endif

cc = meson.get_compiler('c')

if cc.has_function('drmSetClientCap',
                   prefix: '#include <xf86drm.h>',
                   dependencies : [libdrm])
	libhost1x_c_args += '-DHAVE_LIBDRM_CLIENT_CAP'
endif

libhost1x = shared_library('host1x',
	libhost1x_sources,
	include_directories : include_directories('../../include'),
	dependencies : [libhost1x_deps],
	c_args : [libhost1x_c_args],
)
