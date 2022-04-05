DEBUG = -g -D__DEBUG__
REENTRANT = -D__REENTRANT__
CCFLAGS = -D__HAS_FUNC__  ${DEBUG} -I. -I${HOME}/include -DERR_LEVEL ${REENTRANT} 

exe     =
lib     = .so 

CC  = gcc -Wall -fPIC -DLINUX -D__USE_GNU -D_GNU_SOURCE 
CCC = g++ -Wall -fPIC -DLINUX -D__USE_GNU -D_GNU_SOURCE
LDOPT = -L${HOME}/lib -ltbx 

#GTKVER=gtk+-2.0
GTKVER=gtk+-3.0
SRCV=gtksourceview-4
#GTKVER=gtk+-4.0
#SRCV=gtksourceview-5

GTKINC=$(shell pkg-config --cflags ${GTKVER} ${GTKSOURCEVIEW})
GTKLIB=$(shell pkg-config --libs   ${GTKVER} ${GTKSOURCEVIEW})

SRCVINC=$(shell pkg-config --cflags ${SRCV})
SRCVLIB=$(shell pkg-config --libs   ${SRCV})

TARGETS = inv 

all: ${TARGETS}

invsrc = buffer.c inv.c
invobj = ${invsrc:.c=.o}

config:
	@echo "CC      = " ${CC};\
	echo "CCC     = " ${CCC};\
	echo "CCFLAGS = " ${CCFLAGS};\
	echo "LDOPT   = " ${LDOPT};\
	echo "GTKINC  = " ${GTKINC};\
	echo "GTKLIB  = " ${GTKLIB};\
	echo "SRCVINC = " ${SRCVINC};\
	echo "SRCVLIB = " ${SRCVLIB}

clean:
	@echo ">> Clean all <<"; \
    for i in *.o ${TARGETS} ; do [ -f $$i ] && { echo "    removing $$i"; rm $$i; }; done; \
    [ -d db ] && ( cd db; make clean ) ; \
    for i in *.profile; do [ -d $$i ] && { echo "    removing $$i"; rm -rf $$i; }; done; true
	

%.o: %.c 
	@echo -n 	compile "${bold}"$<"${norm}"...; \
	${CC} -c ${CCFLAGS} ${GTKINC} ${SRCVINC} $< && \
	echo "${green}${bold}ok${norm}" || \
	echo "${red}${bold}failed${norm}"

inv: ${invobj}
	@echo -n ">> build ${bold}${blue}"$@"${norm}..."; \
	${CC} -g -o $@ ${LDOPT} ${GTKLIB} ${SRCVLIB} $^  && \
	echo "${green}${bold}ok${norm}" || \
	echo "${red}${bold}failed${norm}"

