# miyoomini/makefile
all: sys emu

sys:
	cd ./lumon && make
	cd ./batmon && make
	cd ./third-party/SDL-1.2 && ./make.sh
	cd ../shared/third-party/DinguxCommander && make -j

emu:
	cd ../shared/third-party/gambatte-dms && PLATFORM=miyoomini ./build_minui.sh
	cd ./third-party/picogpsp && make -f Makefile.miyoomini
	cd ../shared/third-party/pokemini/platform/miyoomini && make -j

clean:
	cd ./lumon && make clean
	cd ./batmon && make clean
	cd ./third-party/SDL-1.2 && make distclean
	cd ../shared/third-party/DinguxCommander && make clean
	cd ../shared/third-party/gambatte-dms && ./clean.sh
	cd ./third-party/picogpsp && make -f Makefile.miyoomini clean
	cd ../shared/third-party/pokemini/platform/miyoomini && make clean
	