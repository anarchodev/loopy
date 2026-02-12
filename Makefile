.PHONY: all clean

all:
	cmake -B build
	cmake --build build

debug:
	cmake -DCMAKE_BUILD_TYPE=DEBUG -B build
	cmake --build build -v

clean:
	rm -rf build
