DEST = ~/.gdb

install: EastlStdContainers.o gdb_stl_utils gdb_tls_utils gdb_checkstack_utils gdb_fiber_utils gdb_log_dump gdb_misc_utils gdb_mem_utils gdb_setup
	@mkdir -p $(DEST)
	cp EastlStdContainers.o $(DEST)/EastlStdContainers.o
	sed -e 's!__DEST__!$(DEST)!' gdb_stl_utils >$(DEST)/gdb_stl_utils
	cp gdb_tls_utils $(DEST)/gdb_tls_utils
	chmod 644 $(DEST)/gdb_tls_utils
	cp gdb_checkstack_utils $(DEST)/gdb_checkstack_utils
	chmod 644 $(DEST)/gdb_checkstack_utils
	cp gdb_fiber_utils $(DEST)/gdb_fiber_utils
	chmod 644 $(DEST)/gdb_fiber_utils
	cp gdb_log_dump $(DEST)/gdb_log_dump
	chmod 644 $(DEST)/gdb_log_dump
	cp gdb_misc_utils $(DEST)/gdb_misc_utils
	chmod 644 $(DEST)/gdb_misc_utils
	cp gdb_mem_utils $(DEST)/gdb_mem_utils
	chmod 644 $(DEST)/gdb_mem_utils
	cp gdb_setup $(DEST)/gdb_setup
	chmod 644 $(DEST)/gdb_setup
	@echo ""
	@echo "Done. You may want to add the following line to your ~/.gdbinit:"
	@echo "         source $(DEST)/gdb_stl_utils"
	@echo "         source $(DEST)/gdb_tls_utils"
	@echo "         source $(DEST)/gdb_checkstack_utils"
	@echo "         source $(DEST)/gdb_fiber_utils"
	@echo "         source $(DEST)/gdb_log_dump"
	@echo "         source $(DEST)/gdb_misc_utils"
	@echo "         source $(DEST)/gdb_mem_utils"
    @echo "Or, just call from within gdb:
	@echo "         source $(DEST)/gdb_setup"

clean:
	rm -f EastlStdContainers.o

uninstall:
	rm -f $(DEST)/EastlStdContainers.o
	rm -f $(DEST)/gdb_stl_utils
	rm -f $(DEST)/gdb_tls_utils
	rm -f $(DEST)/gdb_checkstack_utils
	rm -f $(DEST)/gdb_fiber_utils
	rm -f $(DEST)/gdb_log_dump
	rm -f $(DEST)/gdb_misc_utils
	rm -f $(DEST)/gdb_mem_utils
	rm -f $(DEST)/gdb_setup

EastlStdContainers.o: EastlStdContainers.cpp
	$(error Please build blaze project using nant first)

.PHONY: install
