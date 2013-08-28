# Module.mk for netx module
# Copyright (c) 2000 Rene Brun and Fons Rademakers
#
# Author: G. Ganis, 8/7/2004

MODNAME      := pack
MODDIR       := $(ROOT_SRCDIR)/core/$(MODNAME)
MODDIRS      := $(MODDIR)/src
MODDIRI      := $(MODDIR)/inc

PACKDIR      := $(MODDIR)
PACKDIRS     := $(PACKDIR)/src
PACKDIRI     := $(PACKDIR)/inc

##### libPack #####
PACKL        := $(MODDIRI)/LinkDef.h
PACKDS       := $(call stripsrc,$(MODDIRS)/G__Pack.cxx)
PACKDO       := $(PACKDS:.cxx=.o)
PACKDH       := $(PACKDS:.cxx=.h)

PACKH        := $(filter-out $(MODDIRI)/LinkDef%,$(wildcard $(MODDIRI)/*.h))
PACKS        := $(filter-out $(MODDIRS)/G__%,$(wildcard $(MODDIRS)/*.cxx))
PACKO        := $(call stripsrc,$(PACKS:.cxx=.o))

PACKDEP      := $(PACKO:.o=.d) $(PACKDO:.o=.d)

PACKLIB      := $(LPATH)/libPack.$(SOEXT)
PACKMAP      := $(PACKLIB:.$(SOEXT)=.rootmap)

# used in the main Makefile
ALLHDRS      += $(patsubst $(MODDIRI)/%.h,include/%.h,$(PACKH))
ALLLIBS      += $(PACKLIB)
ALLMAPS      += $(PACKMAP)

# include all dependency files
INCLUDEFILES += $(PACKDEP)

PACKLIBEXTRA =

##### local rules #####
.PHONY:         all-$(MODNAME) clean-$(MODNAME) distclean-$(MODNAME)

include/%.h:    $(PACKDIRI)/%.h
		cp $< $@

$(PACKLIB):     $(PACKO) $(PACKDO) $(ORDER_) $(MAINLIBS) $(PACKLIBDEP)
		@$(MAKELIB) $(PLATFORM) $(LD) "$(LDFLAGS)" \
		   "$(SOFLAGS)" libPack.$(SOEXT) $@ "$(PACKO) $(PACKDO)" \
		   "$(PACKLIBEXTRA)"

$(PACKDS):      $(PACKH) $(PACKL) $(ROOTCINTTMPDEP)
		$(MAKEDIR)
		@echo "Generating dictionary $@..."
		$(ROOTCINTTMP) -f $@ -c $(PACKINCEXTRA) $(PACKH) $(PACKL)

$(PACKMAP):     $(RLIBMAP) $(MAKEFILEDEP) $(PACKL)
		$(RLIBMAP) -o $@ -l $(PACKLIB) -d $(PACKLIBDEPM) -c $(PACKL)

all-$(MODNAME): $(PACKLIB) $(PACKMAP)

clean-$(MODNAME):
		@rm -f $(PACKO) $(PACKDO)

clean::         clean-$(MODNAME)

distclean-$(MODNAME): clean-$(MODNAME)
		@rm -f $(PACKDEP) $(PACKDS) $(PACKDH) $(PACKLIB) $(PACKMAP)

distclean::     distclean-$(MODNAME)

