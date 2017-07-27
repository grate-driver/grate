libhost1x_sources =  files(
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

libhost1x = shared_library('host1x',
	libhost1x_sources,
	include_directories : include_directories('../../include'),
	dependencies : [ libdrm, libpng ]
)
