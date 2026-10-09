
CC=gcc
CFLAGS=-Wall -Wpedantic 
DEBUG_CFLAGS=-g
SOURCE_FILES=src/main.c src/lexer.c

.SHELLFLAGS := -euc

# All are on github
DEPENDENCIES:=nemequ/munit:fbbdf1467eb0d04a6ee465def2e529e4c87f2118 

all: main

.PHONY: debug
debug: CFLAGS += $(DEBUG_CFLAGS)
debug: main

main: 
	if [ ! -d build ]; then mkdir build; fi
	$(CC) $(CFLAGS) $(SOURCE_FILES) -o build/main

build/munit.o:
	$(CC) $(CFLAGS) deps/munit/munit.c -o build/munit.o
	-I deps/munit/munit.h

build/testall: build/munit.o
	$(CC) $(CFLAGS) test/* -o build/tests.o
	$(CC) $(CFLAGS) build/tests.o build/munit.o -o build/test

.PHONY: testall
testall: build/testall
	./build/testall



#.PHONY: dependencies
#dependencies:
#	for dep in "$(DEPENDENCIES)"; do \
#		user=$$(echo "$$dep" | cut -d/ -f1); \
#		repo=$$(echo "$$dep" | cut -d/ -f2 | cut -d: -f1); \
#		hash=$$(echo "$$dep" | cut -d: -f2); \
#		url="https://github.com/$$user/$$repo.git"; \
#		current=$$(git ls-remote "$$url" HEAD | cut -f1); \
#j		if [ "$$current" != "$$hash" ]; then \
#			echo "Not most recent"; \
#		fi; \
#j		if [ ! -d "deps/$$repo" ]; then \
#			mkdir -p "deps/$$repo"; \
#			git clone "$$url" "deps/$$repo"; \
#			cd "deps/$$repo" && git checkout "$$hash"; \
#			cd "../.." \
#		fi \
#	done

.PHONY: clean
clean:
	if [ -d build ]; then rm -r build/*; fi


.PHONY: cleandeps
cleandeps:
	if [ -d deps ]; then rm -r deps/*; fi
