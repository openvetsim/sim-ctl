# Top level makefile
#
# Each subdir, targets for "all", "install" and "clean" should be provided.

# pulse
SUBDIRS =  comm cardiac cpr  respiration pulse wav-trig initialization test www

default:
#	@test -s ../BeagleBoneBlack-GPIO || { echo "Did not find BeagleBoneBlack-GPIO! Exiting..."; exit 1; }

	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir ; \
		done
	
build:
	@mkdir -p build

all clean install :
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir  $@; \
		done
	
.PHONY: build $(SUBDIRS)
