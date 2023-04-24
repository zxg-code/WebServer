all:
	mkdir -p bin
	cd build && make

clean:
	rm -r bin