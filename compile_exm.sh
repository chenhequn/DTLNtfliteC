rm -rf bin
mkdir bin
g++ \
	examples/ns.c \
	-Iinc \
	-Ithirdpart/include \
	-Llib/x86_64 \
	-Lthirdpart/lib/x86_64 \
	-lathena \
	-lsndfile \
	-lpthread \
	-ldl \
	-lm \
	-Wl,-rpath,./lib/x86_64 \
	-no-pie \
	-o bin/dtln
