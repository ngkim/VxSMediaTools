COMPILER=g++

SDK_PATH=include

COMPILER_FLAGS= -Wno-multichar -I$(SDK_PATH) -fno-rtti -O3 

LIBS=-lpthread ${SUN_LIBS} -lm -ldl -lm -ldl

PROJ_EXE=stip_receiver

default: $(PROJ_EXE)

install: default
	cp $(PROJ_EXE) ./bin

stip_receiver: stip_receiver.o
	$(COMPILER) -o $(PROJ_EXE) stip_receiver.o $(LIBS)

stip_receiver.o: stip_receiver.cpp
	$(COMPILER) $(COMPILER_FLAGS) $(INCLUDE_DIR) -c stip_receiver.cpp

clean:
	\rm -f *~ *.o $(PROJ_EXE)
