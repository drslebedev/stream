#use minimum options set

ifneq ($(shell root-config --version 2>/dev/null),)
IS_ROOT=true
endif


CXXOPT        = -O2 -fPIC $(INCLUDES:%=-I%)
LD            = g++
CXX           = g++

NEWLIB = $(STREAMSYS)/lib/libStream.so

NEWLIB_HEADERS = $(wildcard $(STREAMSYS)/include/base/*.h) \
                 $(wildcard $(STREAMSYS)/include/nx/*.h) \
                 $(wildcard $(STREAMSYS)/include/get4/*.h)
                  
NEWLIB_SRCS =    $(wildcard base/*.cxx) \
                 $(wildcard nx/*.cxx) \
                 $(wildcard get4/*.cxx)
                 
NEWLIB_OBJS = $(patsubst %.cxx, %.o, $(NEWLIB_SRCS))
NEWLIB_DEPS = $(patsubst %.cxx, %.d, $(NEWLIB_SRCS))

NEWLIBDICT_LIBNAME = $(STREAMSYS)/lib/libStreamDict
NEWLIBDICT_LIB = $(NEWLIBDICT_LIBNAME).so
NEWLIBDICT_MAP = $(NEWLIBDICT_LIBNAME).rootmap

NEWLIBDICT_LINKDEF = LinkDef.h
NEWLIBDICT_NAME = G__Stream
NEWLIBDICT_HDR = $(NEWLIBDICT_NAME).h
NEWLIBDICT_SRC = $(NEWLIBDICT_NAME).cxx
NEWLIBDICT_OBJ = $(NEWLIBDICT_NAME).o


all: lib dict

lib: $(NEWLIB)

clean:
	rm -f $(NEWLIB) $(NEWLIB_OBJS) $(NEWLIB_DEPS) $(NEWLIBDICT_MAP) $(NEWLIBDICT_NAME).*


ifdef IS_ROOT

$(NEWLIBDICT_MAP): $(NEWLIBDICT_LIB)
	@echo 'Building: $@'
	@rlibmap -o $@ -l $(NEWLIBDICT_LIB) -d $(NEWLIB) -c $(NEWLIBDICT_LINKDEF)

$(NEWLIBDICT_SRC): $(NEWLIBDICT_LINKDEF) $(NEWLIB_HEADERS)
	@rootcint -f $@ -c -p -I../include $(NEWLIB_HEADERS) $(NEWLIBDICT_LINKDEF)

$(NEWLIBDICT_OBJ): CXXOPT += $(shell root-config --cflags)
$(NEWLIBDICT_OBJ): INCLUDES += $(STREAMSYS)/include

$(NEWLIBDICT_LIB): $(NEWLIBDICT_OBJ)
	@echo 'Building: $@'
	@$(LD) -o $@ -shared -O $(NEWLIBDICT_OBJ)

dict: $(NEWLIBDICT_LIB) $(NEWLIBDICT_MAP)

else

dict:
	@echo "ROOT not available, dictionary for $(NEWLIB) cannot be generated"

endif

$(NEWLIB_OBJS): INCLUDES += $(STREAMSYS)/include

$(NEWLIB) : $(NEWLIB_OBJS)
	@echo 'Building: $@'
	$(LD) -shared -Wl,--as-needed -O $(NEWLIB_OBJS) -o $@ 


# rules	
%.d: %.cxx
	@echo "Build dependency for $< ..."
	@rm -f $@ $@.bak; touch $@
	@makedepend -f$@ -Y -o.o -w 30000 -- $(CXXOPT) -- $< > /dev/null 2>&1
	@sed -i 's|$*.o|$*.o $*.d|g' $@
	@rm -f $@.bak

%.o: %.cxx
	$(CXX) -c $< $(CXXOPT) -o $@

ifeq ($(findstring $(MAKECMDGOALS), clean),)
-include $(NEWLIB_DEPS)
endif
	
	
	
	