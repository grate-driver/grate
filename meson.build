project('grate', 'c')

cc = meson.get_compiler('c')
lex = find_program('lex')
yacc = find_program('yacc')
math = cc.find_library('m', required : false)
libdrm = dependency('libdrm')
libpng = dependency('libpng')
devil = dependency('ILU')

egl = dependency('egl', required : false)
x11 = dependency('x11', required : false)
if egl.found() and x11.found()
	gles1 = dependency('glesv1_cm', required : false)
	gles2 = dependency('glesv2', required : false)
endif

subdir('src')
subdir('tools')
subdir('tests')
