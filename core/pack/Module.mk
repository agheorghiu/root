# Module.mk for pack module
#
# Author: Andru Gheorghiu, 29/8/2013

MODNAME      := pack
MODDIR       := $(ROOT_SRCDIR)/core/$(MODNAME)
MODDIRS      := $(MODDIR)/src
MODDIRI      := $(MODDIR)/inc

PACKDIR      := $(MODDIR)
PACKDIRS     := $(PACKDIR)/src
PACKDIRI     := $(PACKDIR)/inc

##### libPack (part of libCore) #####
PACKL        := $(MODDIRI)/LinkDef.h
PACKDS       := $(call stripsrc,$(MODDIRS)/G__Pack.cxx)
PACKDO       := $(PACKDS:.cxx=.o)
PACKDH       := $(PACKDS:.cxx=.h)

PACKH        := $(filter-out $(MODDIRI)/LinkDef%,$(wildcard $(MODDIRI)/*.h))
PACKS        := $(filter-out $(MODDIRS)/G__%,$(wildcard $(MODDIRS)/*.cxx))
PACKO        := $(call stripsrc,$(PACKS:.cxx=.o))

PACKDEP      := $(PACKO:.o=.d) $(PACKDO:.o=.d)

# used in the main Makefile
ALLHDRS     += $(patsubst $(MODDIRI)/%.h,include/%.h,$(PACKH))

# include all dependency files
INCLUDEFILES += $(PACKDEP)

##### local rules #####
.PHONY:         all-$(MODNAME) clean-$(MODNAME) distclean-$(MODNAME)

include/%.h:    $(PACKDIRI)/%.h
		cp $< $@

$(PACKDS):      $(PACKH) $(PACKL) $(ROOTCINTTMPDEP)
		$(MAKEDIR)
		@echo "Generating dictionary $@..."
		$(ROOTCINTTMP) -f $@ -c $(PACKH) $(PACKL)

all-$(MODNAME): $(PACKO) $(PACKDO)

clean-$(MODNAME):
		@rm -f $(PACKO) $(PACKDO)

clean::         clean-$(MODNAME)

distclean-$(MODNAME): clean-$(MODNAME)
		@rm -f $(PACKDEP) $(PACKDS) $(PACKDH)

distclean::     distclean-$(MODNAME)
