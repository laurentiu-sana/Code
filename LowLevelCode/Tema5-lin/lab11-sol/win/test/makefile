#
# test Makefile
#

CFLAGS = /nologo /W3

all: test.exe

test.exe: test.obj
	$(CPP) $(CFLAGS) /Fe$@ $** /link wsock32.lib

clean:
	del *.obj test.exe
