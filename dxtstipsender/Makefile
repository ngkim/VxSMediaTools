COMPILER=g++

SDK_PATH=include

COMPILER_FLAGS= -Wno-multichar -I$(SDK_PATH) -fno-rtti -O3 

LIBS=-lpthread ${SUN_LIBS} -lm -ldl -lm -ldl

PROJ_EXE=dxtstipsender

default: $(PROJ_EXE)

install: default
	cp $(PROJ_EXE) ./bin

dxtstipsender: dxtstipsender.o $(SDK_PATH)/DeckLinkAPIDispatch.o 
	$(COMPILER) -o $(PROJ_EXE) dxtstipsender.o $(SDK_PATH)/DeckLinkAPIDispatch.o $(LIBS)

dxtstipsender.o: dxtstipsender.cpp
	$(COMPILER) $(COMPILER_FLAGS) $(INCLUDE_DIR) -c dxtstipsender.cpp

clean:
	\rm -f *~ *.o $(SDK_PATH)/DeckLinkAPIDispatch.o $(PROJ_EXE)
