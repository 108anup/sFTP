CC = gcc
LD = ld

CLIENT_SRCDIR := client_src
SERVER_SRCDIR := server_src
COMMON_SRCDIR := common_src
BUILDDIR := build
DEPDIR := deps
OUTPUTDIR := bin
TARGET_CLIENT := $(OUTPUTDIR)/client
TARGET_SERVER := $(OUTPUTDIR)/server

VPATH = $(CLIENT_SRCDIR) $(SERVER_SRCDIR) $(COMMON_SRCDIR)

WARNINGS = -Wall -W -Wstrict-prototypes -Wmissing-prototypes -Wsystem-headers
CFLAGS = -g
CPPFLAGS = -I$(CLIENT_SRCDIR) -I$(SERVER_SRCDIR) -I$(COMMON_SRCDIR)
LDFLAGS =
LDLIB =
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

CLIENT_SOURCES = $(shell find $(CLIENT_SRCDIR) -type f -name "*.c")
SERVER_SOURCES = $(shell find $(SERVER_SRCDIR) -type f -name "*.c")
COMMON_SOURCES = $(shell find $(COMMON_SRCDIR) -type f -name "*.c")

CLIENT_OBJECTS = $(patsubst $(CLIENT_SRCDIR)/%,$(BUILDDIR)/%,$(CLIENT_SOURCES:.c=.o))
SERVER_OBJECTS = $(patsubst $(SERVER_SRCDIR)/%,$(BUILDDIR)/%,$(SERVER_SOURCES:.c=.o))
COMMON_OBJECTS = $(patsubst $(COMMON_SRCDIR)/%,$(BUILDDIR)/%,$(COMMON_SOURCES:.c=.o))

DEPS = $(patsubst $(CLIENT_SRCDIR)/%,$(DEPDIR)/%,$(CLIENT_SOURCES:.c=.d))
DEPS += $(patsubst $(SERVER_SRCDIR)/%,$(DEPDIR)/%,$(SERVER_SOURCES:.c=.d))
DEPS += $(patsubst $(COMMON_SRCDIR)/%,$(DEPDIR)/%,$(COMMON_SOURCES:.c=.d))

OUTPUT_OPTION = -o $@
COMPILE.c = $(CC) -c $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(WARNINGS)
POSTCOMPILE = @mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d && touch $@

$(shell mkdir -p $(DEPDIR) > /dev/null)
$(shell mkdir -p $(BUILDDIR) > /dev/null)
$(shell mkdir -p $(OUTPUTDIR) > /dev/null)

all: build link

dependencies:

build: $(CLIENT_OBJECTS) $(COMMON_OBJECTS) $(SERVER_OBJECTS)
link: $(TARGET_CLIENT) $(TARGET_SERVER)

.PHONY: clean TAGS install cscope

$(TARGET_CLIENT): $(CLIENT_OBJECTS) $(COMMON_OBJECTS)
	$(CC) $(LDFLAGS) $(OUTPUT_OPTION) $^ $(LDLIB)


$(TARGET_SERVER): $(SERVER_OBJECTS) $(COMMON_OBJECTS)
	$(CC) $(LDFLAGS) $(OUTPUT_OPTION) $^ $(LDLIB)

%.o: %.c
$(BUILDDIR)/%.o: $(CLIENT_SRCDIR)/%.c $(DEPDIR)/%.d
	$(COMPILE.c) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

$(BUILDDIR)/%.o: $(SERVER_SRCDIR)/%.c $(DEPDIR)/%.d
	$(COMPILE.c) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

$(BUILDDIR)/%.o: $(COMMON_SRCDIR)/%.c $(DEPDIR)/%.d
	$(COMPILE.c) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

$(DEPDIR)/%.d: ;
.PRECIOUS: $(DEPDIR)/%.d

-include $(DEPS)

TAGS:
	find . -name "*.[chS]" | xargs etags -a

cscope:
	find . -name "*.[chS]" > cscope.files
	cscope -b -q -k

clean:
	$(RM) -r -v $(BUILDDIR) $(OUTPUTDIR) $(DEPDIR)

cleanall: clean
	$(RM) -v cscope.* TAGS

help:
	@echo 'make <all, clean, build, link, dependencies, TAGS, cscope>'
