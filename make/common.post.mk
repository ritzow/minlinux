.SECONDARY:

PROJ_DIR=$(OUTPUT_DIR)/$(project)

.PHONY: $(project)
$(project): \
	project-deps \
	$(PROJ_DIR) \
	$(addprefix  $(PROJ_DIR)/,$(objects)) \
	$(addprefix  $(PROJ_DIR)/,$(programs))

.PHONY: project-deps
project-deps:
	$(foreach proj,$(project-deps),$(MAKE) -C $(PROJECT_DIR) -f projects.mk $(proj))

.SECONDEXPANSION:

$(PROJ_DIR) : $(OUTPUT_DIR)
	mkdir -p $(PROJ_DIR)

$(OUTPUT_DIR) :
	mkdir -p $(OUTPUT_DIR)

$(PROJ_DIR)/%.o : $$(%.o-deps) $$($$(addsuffix -deps,$$(%.o-deps)))
	$(CC) $(CFLAGS) -c $($*.o-deps) -o $@

#$(PROJ_DIR)/%.a : #$*.c $$(%.c-deps) common.mk
#	$(error "Static library building not implemented!")

$(PROJ_DIR)/% : $$(addprefix $(OUTPUT_DIR)/,$$(%-deps))
	$(CC) $(CFLAGS) $^ -o $@

.DEFAULT:
	$(error Unknown target "$@")