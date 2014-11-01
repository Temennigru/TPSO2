# project name (generate executable with this name)
TARGET   = tp2

CC       = gcc -std=c11 -c
# compiling flags here
CFLAGS   = -Wall -I. -Wextra -lpthread -DNDEBUG

LINKER   = gcc -o
# linking flags here
LFLAGS   = -Wall -Wextra

# debug flags here
DFLAGS   = -g -DDEBUG

SOURCES  := $(wildcard *.c)
INCLUDES := $(wildcard *.h)
OBJECTS  := $(SOURCES:.c=*.o)
rm       = rm -rf

$(TARGET): obj
@$(LINKER) $(TARGET) $(LFLAGS) $(OBJECTS)
@echo "Linking complete!"

obj: $(SOURCES) $(INCLUDES)
@$(CC) $(CFLAGS) -DNDEBUG $(SOURCES)
@echo "Compilation complete!"

#debug:
#	gcc $(DFLAGS) $(SOURCES) -o $(TARGET)

dobj: $(SOURCES) $(INCLUDES)
@$(CC) $(CFLAGS) $(DFLAGS) $(SOURCES)
@echo "dlinking complete!"

debug: dobj
@$(LINKER) $(TARGET) $(LFLAGS) $(DFLAGS) $(OBJECTS) -o $(TARGET)
@echo "dcompilation complete!"

clean:
@$(rm) $(TARGET) $(OBJECTS) *.dSYM
@echo "Cleanup complete!"
