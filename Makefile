TARGET_EXEC := Stronk

BUILD_DIR := ./build
SRC_DIRS := ./src ./lib
TEST_SRC_DIRS := ./tests

# Find all the C files we want to compile
# Note the single quotes around the * expressions. 
# The shell will incorrectly expand these otherwise, but we want to send the * directly to the find command.
SRCS := $(shell find $(SRC_DIRS) ! -name main.c -name '*.c' -or -name '*.s')
MAIN_SRCS := src/main.c
TEST_SRCS := $(shell find $(TEST_SRC_DIRS) -name '*.c' -or -name '*.s')

# Prepends BUILD_DIR and appends .o to every src file
# As an example, ./your_dir/hello.cpp turns into ./build/./your_dir/hello.cpp.o
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
MAIN_OBJS := $(MAIN_SRCS:%=$(BUILD_DIR)/%.o)
TEST_OBJS := $(TEST_SRCS:%=$(BUILD_DIR)/%.o)

# Every folder in ./src will need to be passed to GCC so that it can find header files
INC_DIRS := $(shell find $(SRC_DIRS) -type d)
TEST_INC_DIRS := $(shell find $(TEST_SRC_DIRS) -type d)
# Add a prefix to INC_DIRS. So moduleA would become -ImoduleA. GCC understands this -I flag
INC_FLAGS := $(addprefix -I,$(INC_DIRS))
TEST_INC_FLAGS := $(addprefix -I,$(TEST_INC_DIRS))

# The -MMD and -MP flags together generate Makefiles for us!
# These files will have .d instead of .o as the output.
CFLAGS := -D__FILENAME__='""' \
	$(INC_FLAGS) \
	-std=c2x \
	-pedantic \
	-ggdb \
	-O0 \
	-ftrapv \
	-fno-stack-protector \
	-pipe \
	-Wall \
	-Wextra \
	-Wno-unused-function \
	-Wno-pointer-arith \
	-Wno-unused-parameter \
	-Wno-unused-variable \
	-D_POSIX_C_SOURCE \
	-DDEBUG \
	-DHAVE_FOPENCOOKIE \
	-DHAVE_LIBUUID \
	-DNANOSLEEP_HACKY_FIX \
	-DFORCE_C11_THREAD_EMULATION \
	-D_REENTRANT \
	-D_POSIX_C_SOURCE \
	-D_GNU_SOURCE \
	-DHAVE_ASPRINTF \
	-pthread

LDFLAGS := -lcrypto \
	-lssl \
	-lz \
	-lcurl \
	-lzlog

# The final build step.
$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS) $(MAIN_OBJS)
	$(CXX) $(OBJS) $(MAIN_OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/test: $(OBJS) $(TEST_OBJS)
	$(CXX) $(OBJS) $(TEST_OBJS) -o $@ $(LDFLAGS) -lcmocka

# Build step for C source
$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	gcc $(CFLAGS) -c $< -o $@

.PHONY: server
server: $(BUILD_DIR)/$(TARGET_EXEC)

.PHONY: test
test: $(BUILD_DIR)/test

.PHONY: clean
clean:
	rm -r $(BUILD_DIR)
