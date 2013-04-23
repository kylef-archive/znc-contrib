MC=znc-buildmod

MODEXTEN=.so
SRCEXTEN=.cpp

SRCFILES=clientaway.cpp ignore.cpp privmsg.cpp

MODDIR=.znc/modules

all: 
	@$(MC) $(SRCFILES)
	@mv $(SRCFILES:$(SRCEXTEN)=$(MODEXTEN)) $(MODDIR)
	@echo All modules have been built and placed in $(MODDIR)

%:
	@:
		@$(MC) $(filter-out $@,$(MAKECMDGOALS)$(SRCEXTEN))
		@mv $(filter-out $@,$(MAKECMDGOALS)$(MODEXTEN)) $(MODDIR)
		@echo The module has now been built and placed in $(MODDIR)

clean:
	@rm -fr $(MODDIR)/*$(MODEXTEN)
	@echo All modules have been removed from $(MODDIR)

