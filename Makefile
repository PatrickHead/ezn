CC = x86_64-w64-mingw32-gcc

EXECUTABLE = ezn.exe
COPTS = -O3 -g0 -Wall
LOPTS =

TESTDIR = ./tests
DEVDIR = ~/w1/Develop
BKDIR = ${DEVDIR}/ezn

all: $(EXECUTABLE) #gui.build

$(EXECUTABLE): ezn.o mkdir_p.o file_info.o which.o
	@echo Building $(EXECUTABLE)
	@$(CC) $(COPTS) $(LOPTS) -o $(EXECUTABLE) ezn.o mkdir_p.o file_info.o which.o

ezn.o: ezn.c file_info.h mkdir_p.h
	@$(CC) $(COPTS) -c ezn.c

mkdir_p.o: mkdir_p.c
	@$(CC) $(COPTS) -c mkdir_p.c

which.o: which.c which.h file_info.h
	@$(CC) $(COPTS) -c which.c

file_info.o: file_info.c
	@$(CC) $(COPTS) -c file_info.c

gui.build:
	@(cd gui; make --no-print-directory)

strip: $(EXECUTABLE)
	strip -s $(EXECUTABLE)

clean:
	@(rm -f *.o)
	@(rm -f $(EXECUTABLE))
	@(rm -rf ${TESTDIR}/*)
	@(cd gui; make --no-print-directory clean)

${TESTDIR}:
	@mkdir -p ${TESTDIR}

${DEVDIR}:
	@mw

${BKDIR}: ${DEVDIR}
	@mkdir -p ${BKDIR}

backup: clean ${BKDIR}
	@cp -aur . ${BKDIR}

commit:
	git commit -a -C master

push:
	git push origin master

