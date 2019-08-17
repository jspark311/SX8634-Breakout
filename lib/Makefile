###########################################################################
# Makefile for libraries
# Author: J. Ian Lindsay
###########################################################################

LIBS_TO_BUILD  = manuvr.a


###########################################################################
# Option conditionals
###########################################################################

ifeq ($(SECURE),1)
	# Linux will require that we build mbedtls, since it won't
	# come with the build otherwise.
	LIBS_TO_BUILD += mbedtls.a
endif


###########################################################################
# Parameter unification and make targets
###########################################################################

.PHONY: all


#all: $(LIBS_TO_BUILD)
all: mbedtls.a	manuvr.a
	@echo "\033[1;37m$(LIBS_TO_BUILD)\033[0m"


mbedtls.a:
	@echo "\033[1;37m=== mbedtls ==========================================\033[0m"
	$(MAKE) lib -C mbedtls/
	cp -RP mbedtls/library/*.a $(OUTPUT_PATH)


manuvr.a:
	$(MAKE) -C ManuvrOS/ManuvrOS


clean:
	$(MAKE) clean -C ManuvrOS/ManuvrOS
	$(MAKE) clean -C mbedtls
