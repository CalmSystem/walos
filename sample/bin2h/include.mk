BIN2H := $(BUILD_DIR)sample/bin2h
$(BIN2H): $(ROOT_DIR)sample/bin2h/main.c
	$(MKDIR_P) $(dir $@)
	$(CC) $^ -o $@
