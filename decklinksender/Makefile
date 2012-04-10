COMPILER=g++

SDK_PATH=include

COMPILER_FLAGS= -Wno-multichar -I$(SDK_PATH) -fno-rtti -O3 

LIBS=-lpthread ${SUN_LIBS} -lm -ldl -lm -ldl

PROJ_EXE=decklinksender

default: $(PROJ_EXE)

install: default
	cp $(PROJ_EXE) ./bin

decklinksender: decklinksender.o $(SDK_PATH)/DeckLinkAPIDispatch.o 
	$(COMPILER) -o $(PROJ_EXE) decklinksender.o $(SDK_PATH)/DeckLinkAPIDispatch.o $(LIBS)

decklinksender.o: decklinksender.cpp
	$(COMPILER) $(COMPILER_FLAGS) $(INCLUDE_DIR) -c decklinksender.cpp

clean:
	\rm -f *~ *.o $(SDK_PATH)/DeckLinkAPIDispatch.o $(PROJ_EXE)
