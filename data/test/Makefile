EXEC = test
SRCS = test.cpp
OBJS = test.o

.SILENT:

.DEFAULT_GOAL: test

test: $(OBJS)
	@echo "Linking test"
	g++ -o $(EXEC) $(OBJS)

CFLAGS = -I.

$(OBJS): $(SRCS)
	@echo "Compiling $<"
	g++ -c -o $(OBJS) $(CFLAGS) $(SRCS)

clean:
	rm $(EXEC) $(OBJS)

print: *.cpp
	echo $?
