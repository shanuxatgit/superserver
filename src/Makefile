#CC             = /usr/local/gcc-4.1.1/bin/gcc -W -Wall -Wstrict-prototypes -Wextra -Wno-pointer-sign -Wstrict-prototypes
CC		= g++
#CXXFLAGS	= -W -Wall -g -g3 -DDEBUG -D_GNU_SOURCE -DMYSQLPP_MYSQL_HEADERS_BURIED -D_XOPEN_SOURCE -Wunused-but-set-variable -Wunused-function
CXXFLAGS	= -W -Wall -g -g3 -DDEBUG -D_GNU_SOURCE -DMYSQLPP_MYSQL_HEADERS_BURIED -D_XOPEN_SOURCE 
INCLUDES	= -I/usr/local/include -I /usr/src/
LIBS		+= -lpthread -lz -lm -L/usr/local/lib -lpcrecpp -lmysqlpp -lcrypt
FLAGS		+= -DDEBUG -D_GNU_SOURCE -DMYSQLPP -DMYSQLPP_MYSQL_HEADERS_BURIED -D_XOPEN_SOURCE

MAIN_SRC := superserver.cpp

SRC +=  tpool.cpp \
	util.cpp \
	conf.cpp \
	common.cpp \
	log.cpp \
	rc_server1.cpp \
	mng_server1.cpp \
	php_server1.cpp \
	pms_server1.cpp \
	his_server.cpp \
	client_server.cpp

EXE := $(basename $(MAIN_SRC))

MAIN_OBJ := $(addsuffix .o, $(basename $(MAIN_SRC)))
OBJ := $(addsuffix .o, $(basename $(SRC)))

EXECUTABLE = $(CC) $(INCLUDES) $(FLAGS) -o $@ $^ $(LIBS) && chmod a+x $@
#EXECUTABLE = g++ $^ $(LIBS) && chmod a+x $@  

.PHONY: clean all

#all: $(EXE)
all: $(EXE)


$(EXE): $(MAIN_OBJ) $(OBJ)
	$(EXECUTABLE)

clean  :
	$(RM) $(OBJ) $(MAIN_OBJ) $(EXE) *~ *.gdb

radiosrv: $(MAIN_SRC) $(SRC)
	${CC} ${INCLUDES} ${LIBS} ${FLAGS} $(MAIN_SRC) $(SRC) -o $(EXE)


