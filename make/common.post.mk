.PHONY: $(project)

PROJ_DIR=$(OUTPUT_DIR)/$(project)

$(project): $(PROJ_DIR) $(addprefix  $(PROJ_DIR)/,$(objects)) #TODO add programs
#	$(info Built project "$(project)")

$(PROJ_DIR) : $(OUTPUT_DIR)
	mkdir -p $(PROJ_DIR)

$(OUTPUT_DIR) :
	mkdir -p $(OUTPUT_DIR)

.SECONDEXPANSION:

$(PROJ_DIR)/%.o : $$(%.o-deps) $$($$(addsuffix -deps,$$(%.o-deps)))
	$(CC) $(CFLAGS) -c $($*.o-deps) -o $@

$(PROJ_DIR)/%.a : #$*.c $$(%.c-deps) common.mk
	$(error "Static library building not implemented!")
#$(CC) $(CFLAGS) -c $*.c -o $@

#%.c : $$(%.c-deps) zooptest
#	@echo "Made $@ with $^"

.PHONY: %.h
%.h : #$$(%.h-deps) #TODO recursive header deps $$(foreach)
	@echo Made $@

#$(PROJ_DIR)/% : $$(%-deps)
#	$(CC) $(CFLAGS) $*.o -o $*

.DEFAULT:
	$(error Unknown target "$@")

#.PHONY: %