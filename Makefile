.DELETE_ON_ERROR:

# You can override this to the location of `catch.hpp`.
CATCH_INCLUDE_DIR ?= /usr/include

# You can override this to `cat` if you don't have clang-format.
CLANG_FORMAT ?= clang-format -style=file

# This is only used for the tests, so there's no real need to tinker with it.
COMPILE ?= g++ --std=gnu++1y -g

# Extract a version string from GIT. Dirty state gets a +1 bump on the patch
# version.
GIT_VERSION=`git describe --always --dirty --tags | perl -pe 's/-(\d*)-(.*)-dirty/".".($$1+1)/e' | sed 's/-\(.*\)-.*/.\1/'`

.PHONY: all
all: test html

.PHONY: test test.fast test.safe
test: test.fast test.safe
test.fast: bin/.tested.fast
test.safe: bin/.tested.safe

.PHONY: src
src:
	mv cpl.hpp cpl.hpp.before
	cp cpl.hpp.before cpl.hpp
	$(CLANG_FORMAT) cpl.hpp.before > cpl.hpp.after
	export CPL_VERSION="$(GIT_VERSION)"; sed -i "s/CPL_VERSION.*/CPL_VERSION \"$$CPL_VERSION\"/" cpl.hpp.after
	if cmp -s cpl.hpp.before cpl.hpp.after; \
	then rm cpl.hpp; mv cpl.hpp.before cpl.hpp; rm cpl.hpp.after; \
	else rm cpl.hpp; mv cpl.hpp.after cpl.hpp; rm cpl.hpp.before; \
	fi
	mv test.cpp test.cpp.before
	clang-format -style=file test.cpp.before > test.cpp.after
	if cmp -s test.cpp.before test.cpp.after; \
	then mv test.cpp.before test.cpp; rm test.cpp.after; \
	else mv test.cpp.after test.cpp; rm test.cpp.before; \
	fi

bin:
	mkdir -p bin

bin/test.fast: test.cpp cpl.hpp | src bin
	$(COMPILE) -DCPL_FAST -I$(CATCH_INCLUDE_DIR) -o $@ $<

bin/test.safe: test.cpp cpl.hpp | src bin
	$(COMPILE) -DCPL_SAFE -I$(CATCH_INCLUDE_DIR) -o $@ $<

bin/.tested.fast: bin/test.fast
	$<
	touch $@

bin/.tested.safe: bin/test.safe
	$<
	touch $@

.PHONY: html
html: html/index.html

html/index.html: .doxygen.cfg cpl.hpp test.cpp README.md | src
	rm -rf html
	export CPL_VERSION="$(GIT_VERSION)"; doxygen .doxygen.cfg

clean:
	rm -rf bin html
