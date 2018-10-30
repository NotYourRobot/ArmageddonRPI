CC = gcc
LIBS=-rdynamic -lm -lexpat -lpthread
SHAREDFLAGS=-g3 -fshort-enums
CFLAGS = -std=gnu99 $(PROFILE)  
CFLAGS += $(SHAREDFLAGS) 
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
dep = $(OBJ:.o=.d)
      
default: easyArm
all: default

easyArm: $(OBJ)
	$(CC) -o $@ $^ $(LIBS)
	
-include $(dep)

%.d: %.c
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

.PHONY: clean
clean:
	-rm -f $(OBJ) easyArm $(dep)


		