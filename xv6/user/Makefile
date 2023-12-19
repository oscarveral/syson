UPROGS=\
	cat\
	echo\
	forktest\
	grep\
	init\
	kill\
	ln\
	ls\
	mkdir\
	rm\
	sh\
	stressfs\
	usertests\
	wc\
	zombie\
	date\
	dup2test\
	exitwait\
	tsbrk1\
	tsbrk2\
	tsbrk3\
	tsbrk4\
	tsbrk5\
	tsbrk6\
	tsbrk7\
	tprio1\
	tprio2\
	tprio3\
	
# --- Boletín 1. Ejercicio 1. --- */
# Se añade el programa date.c para compilar.

# --- Boletín 1. Ejercicio 2. --- */
# Se añade el programa dup2test.c para compilar.

# --- Boletín 1. Ejercicio 3. --- */
# Se añade el programa exitwait.c para compilar.

# --- Boletín 2. Ejercicios 1 y 2. --- */
# Se añaden los programas tsbrk1.c, tsbrk2.c, tsbrk3.c, tsbrk4.c, tsbrk5.c, tsbrk6.c y tsbrk7.c para compilar.

# --- Boletín 3. Ejercicio 2. --- */
# Se añaden los programas tprio1.c, tprio2.c, tprio3.c para compilar.

# Try to infer the correct TOOLPREFIX if not set
ifndef TOOLPREFIX
TOOLPREFIX := $(shell if i386-jos-elf-objdump -i 2>&1 | grep '^elf32-i386$$' >/dev/null 2>&1; \
	then echo 'i386-jos-elf-'; \
	elif objdump -i 2>&1 | grep 'elf32-i386' >/dev/null 2>&1; \
	then echo ''; \
	else echo "***" 1>&2; \
	echo "*** Error: Couldn't find an i386-*-elf version of GCC/binutils." 1>&2; \
	echo "*** Is the directory with i386-jos-elf-gcc in your PATH?" 1>&2; \
	echo "*** If your i386-*-elf toolchain is installed with a command" 1>&2; \
	echo "*** prefix other than 'i386-jos-elf-', set your TOOLPREFIX" 1>&2; \
	echo "*** environment variable to that prefix and run 'make' again." 1>&2; \
	echo "*** To turn off this error, run 'gmake TOOLPREFIX= ...'." 1>&2; \
	echo "***" 1>&2; exit 1; fi)
endif


CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)gas
AR = $(TOOLPREFIX)ar
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump
STRIP = $(TOOLPREFIX)strip
CPPFLAGS = -I. -I..
CFLAGS = -fno-pic -static -fno-builtin -fno-strict-aliasing -Og -Wall -MD -ggdb -march=i386 -m32 -Werror -Wno-infinite-recursion -fno-omit-frame-pointer
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)
ASFLAGS = -m32 -gdwarf-2 -Wa,-divide
# FreeBSD ld wants ``elf_i386_fbsd''
LDFLAGS += -m $(shell $(LD) -V | grep elf_i386 2>/dev/null | head -n 1)

# Disable PIE when possible (for Ubuntu 16.10 toolchain)
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]no-pie'),)
CFLAGS += -fno-pie -no-pie
endif
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]nopie'),)
CFLAGS += -fno-pie -nopie
endif

# Disable implicit rule
%: %.c

%: %.o libc.a
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $@ $^
	$(OBJDUMP) -S $@ > $*.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $*.sym
	$(STRIP) --remove-section=.note.gnu.property $@


all: $(UPROGS)

ULIB = ulib.o usys.o printf.o umalloc.o

libc.a: $(ULIB)
	$(AR) rcs -o $@ $^

forktest: forktest.o libc.a
# forktest has less library code linked in - needs to be small
# in order to be able to max out the proc table.
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $@ forktest.o libc.a
	$(OBJDUMP) -S $@ > forktest.asm
	$(STRIP) --remove-section=.note.gnu.property $@

# Prevent deletion of intermediate files, e.g. cat.o, after first build, so
# that disk image changes after first build are persistent until clean.  More
# details:
# http://www.gnu.org/software/make/manual/html_node/Chained-Rules.html
.PRECIOUS: %.o

clean:
	rm -f *.tex *.dvi *.idx *.aux *.log *.ind *.ilg \
	*.o *.d *.asm *.sym \
	libc.a $(UPROGS)

print-uprogs:
	@echo $(UPROGS)

.PHONY: all clean print-uprogs
