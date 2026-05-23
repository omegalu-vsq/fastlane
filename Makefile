CC = g++

CPP_FILES = video.cpp engine.cpp texture.cpp road.cpp
H_FILES  = video.hh engine.hh texture.hh camera.hh road.hh

ifeq ($(LEGACY),1)
CPP_FILES += engineLegacy.cpp
CXX_FLAGS += -DLEGACY
endif

OBJ_FILES = $(CPP_FILES:.cpp=.o)

CXX_FLAGS += -Wall -Wextra -O3 -g -std=c++20
CXX_FLAGS += -lm -lSDL2
CXX_FLAGS += -march=native
#CXX_FLAGS += -fopt-info-vec-optimized -fopt-info-vec-missed -ftree-vectorize
LDXX_FLAGS =

#For gcc 4.9
#CXXFLAGS+=-fdiagnostics-color=auto
export GCC_COLORS=1

define color
    if test -n "${TERM}" ; then\
	if test `tput colors` -gt 0 ; then \
	    tput setaf $(1); \
        fi;\
    fi
endef

define default_color
    if test -n "${TERM}" ; then\
	if test `tput colors` -gt 0 ; then  tput sgr0 ; fi; \
    fi
endef


all: post-build

pre-build:
	@$(call color,4)
	@echo "******** Starting Compilation ************"
	@$(call default_color)

post-build:
	@make --no-print-directory main-build ; \
	sta=$$?;	  \
	$(call color,4); \
	echo "*********** End Compilation **************"; \
	$(call default_color); \
	exit $$sta;

main-build: pre-build build

build: $(OBJ_FILES)
	$(CC) main.cpp -o fastlane $(OBJ_FILES) $(CXX_FLAGS) $(LDXX_FLAGS)

%.o: %.cpp $(H_FILES)
	@$(call color,2)
	@echo "[$@] $(CXX_FLAGS)"
	@$(call default_color)
	@$(CC) -c -o $@ $< $(CXX_FLAGS) ; \
	sta=$$?;	  \
	if [ $$sta -eq 0 ]; then  \
	  $(call color,2) ; \
	  echo "[$@ succes]" ; \
	  $(call default_color) ; \
	else  \
	  $(call color,1) ; \
	  echo "[$@ failure]" ; \
	  $(call default_color) ; \
	fi ;\
	exit $$sta

.PHONY: all legacy clean pre-build post-build main-build build

legacy:
	@$(MAKE) LEGACY=1

clean:
	rm -f $(OBJ_FILES) engineLegacy.o fastlane
