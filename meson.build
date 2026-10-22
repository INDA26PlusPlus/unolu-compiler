project(
  'unolu-compiler',
  'c',
  meson_version : '>= 1.3.0',
  version : '0.1',
  default_options : ['warning_level=1', 'c_std=gnu17'],
)


sources = [
  'src/main.c',
  'src/lexer.c',
  'src/parser.c',
  'src/symbols.c',
  'src/codegen.c',
]

arguments = []

if get_option('popt')
  popt_command = ['nim', 'c', '--app:staticLib', '-o:./popt.a', ]
  if get_option('buildtype') == 'release'
    popt_command += '-x:off'
    popt_command += '-a:off'
    popt_command += '-d:release'
    popt_command += '--opt:speed'
    popt_command += '--threads:on'
  else
    popt_command += '-d:debug'
  endif
  popt_command += '../src/popt.nim'
  sources += custom_target('poptimizer', output: 'popt.a', command: popt_command, input: [ 'src/popt.h', 'src/popt.nim' ])
  arguments += '-DPOPTIMIZER'
endif

exe = executable(
  'einaplc',
  sources,
  install : true,
  c_args: arguments,
)

test('lexer', executable('test_lexer', ['test/test_lexer.c', 'src/lexer.c']), protocol: 'tap', suite: ['lexer'] )

