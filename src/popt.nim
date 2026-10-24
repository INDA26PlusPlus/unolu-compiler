import std/re

proc popt_nim(path: cstring) {.exportc.} =
  var contents: string = readFile($path)
  let
    pattern_push_pop: Regex = re(r"((?:push .*\n)*)push (%\w{3}|\$-?\d+)\npop (%\w{3})\n((?:pop .*\n)*)", flags = {reMultiLine}) 
    pattern_redundant_move: Regex = re(r"mov(\w) %(\w{3}), %(\w{3})\nmov\w %\3, %\2\n", flags = {reMultiLine}) 
    pattern_self_move: Regex = re"mov(\w) %(\w{3}), %\2"
  while contents.contains(pattern_self_push_pop):
    contents = contents.replacef(pattern_self_push_pop, "$2")
  while contents.contains(pattern_push_pop):
    contents = contents.replacef(pattern_push_pop, "movq $2, $3\n$1$4")
  while contents.contains(pattern_redundant_move):
    contents = contents.replacef(pattern_redundant_move, "mov$1 %$2, %$3\n")
  contents = contents.replace(pattern_self_move)
  writeFile($path, contents)



   
