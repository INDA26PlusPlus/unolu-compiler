project(
  'unolu-compiler',
  'c',
  meson_version : '>= 1.3.0',
  version : '0.1',
  default_options : ['warning_level=1'],
)


sources = [
  'src/main.c',
  'src/lexer.c',
]

exe = executable(
  'main',
  sources,
  install : true,
)

test_sources = [
  'test/main.c',
  'test/lexer.c',
  'src/lexer.c',
]

test('lexer', executable('test_lexer', ['test/test_lexer.c', 'src/lexer.c']), protocol: 'tap', suite: ['lexer'] )


