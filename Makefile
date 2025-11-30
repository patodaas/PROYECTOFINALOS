# Makefile para Storage Manager - Partes 6-10
# Compilador y flags

CC = gcc
CFLAGS = -Wall -Wextra -g -pthread -I./include
LDFLAGS = -lpthread -lsqlite3 -lssl -lcrypto -lm

# Directorios
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
BIN_DIR = bin
KERNEL_DIR = kernel_module

# Archivos fuente (solo partes 6-10)
SOURCES = $(SRC_DIR)/monitor.c \
          $(SRC_DIR)/backup_engine.c \
          $(SRC_DIR)/performance_tuner.c \
          $(SRC_DIR)/ipc_server.c \
          $(SRC_DIR)/utils.c

# Archivos objeto
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCES))

# Ejecutables
DAEMON = $(BIN_DIR)/storage_daemon
CLI = $(BIN_DIR)/storage_cli
TEST_MONITOR = $(BIN_DIR)/test_monitor
TEST_BACKUP = $(BIN_DIR)/test_backup
TEST_PERF = $(BIN_DIR)/test_perf
TEST_IPC = $(BIN_DIR)/test_ipc

# Targets principales
.PHONY: all clean dirs kernel install test help

all: dirs $(DAEMON) $(CLI) $(TEST_MONITOR) $(TEST_BACKUP) $(TEST_PERF) $(TEST_IPC)

# Crear directorios necesarios
dirs:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)
	@mkdir -p /var/lib/storage_mgr 2>/dev/null || true
	@mkdir -p /backup 2>/dev/null || true

# Regla para compilar archivos .c a .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Daemon principal (simplificado para testing)
$(DAEMON): $(OBJECTS) $(SRC_DIR)/daemon_main.c
	@echo "Building daemon..."
	$(CC) $(CFLAGS) $(SRC_DIR)/daemon_main.c $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "✓ Daemon built: $@"

# Cliente CLI
$(CLI): $(OBJECTS) cli/storage_cli.c
	@echo "Building CLI client..."
	$(CC) $(CFLAGS) cli/storage_cli.c $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "✓ CLI built: $@"

# Tests individuales
$(TEST_MONITOR): $(OBJ_DIR)/monitor.o $(OBJ_DIR)/utils.o tests/test_monitor.c
	@echo "Building monitor test..."
	$(CC) $(CFLAGS) tests/test_monitor.c $(OBJ_DIR)/monitor.o $(OBJ_DIR)/utils.o -o $@ $(LDFLAGS)
	@echo "✓ Monitor test built: $@"

$(TEST_BACKUP): $(OBJ_DIR)/backup_engine.o $(OBJ_DIR)/utils.o tests/test_backup.c
	@echo "Building backup test..."
	$(CC) $(CFLAGS) tests/test_backup.c $(OBJ_DIR)/backup_engine.o $(OBJ_DIR)/utils.o -o $@ $(LDFLAGS)
	@echo "✓ Backup test built: $@"

$(TEST_PERF): $(OBJ_DIR)/performance_tuner.o $(OBJ_DIR)/utils.o tests/test_perf.c
	@echo "Building performance test..."
	$(CC) $(CFLAGS) tests/test_perf.c $(OBJ_DIR)/performance_tuner.o $(OBJ_DIR)/utils.o -o $@ $(LDFLAGS)
	@echo "✓ Performance test built: $@"

$(TEST_IPC): $(OBJ_DIR)/ipc_server.o $(OBJ_DIR)/performance_tuner.o $(OBJ_DIR)/utils.o tests/test_ipc.c
	@echo "Building IPC test..."
	$(CC) $(CFLAGS) tests/test_ipc.c $(OBJ_DIR)/ipc_server.o $(OBJ_DIR)/performance_tuner.o $(OBJ_DIR)/utils.o -o $@ $(LDFLAGS)
	@echo "✓ IPC test built: $@"


# Compilar módulo del kernel
kernel:
	@echo "Building kernel module..."
	@cd $(KERNEL_DIR) && $(MAKE)
	@echo "✓ Kernel module built"

# Instalar (requiere root)
install: all kernel
	@echo "Installing storage manager..."
	sudo cp $(DAEMON) /usr/local/bin/
	sudo cp $(CLI) /usr/local/bin/
	sudo mkdir -p /var/lib/storage_mgr
	sudo mkdir -p /backup
	@echo "✓ Installation complete"
	@echo ""
	@echo "To load kernel module:"
	@echo "  cd $(KERNEL_DIR) && sudo make install"

# Tests
test: $(TEST_MONITOR) $(TEST_BACKUP) $(TEST_PERF) $(TEST_IPC)
	@echo ""
	@echo "=== Running Tests ==="
	@echo ""
	@echo "1. Monitor Test:"
	@sudo $(TEST_MONITOR) || true
	@echo ""
	@echo "2. Backup Test:"
	@sudo $(TEST_BACKUP) || true
	@echo ""
	@echo "3. Performance Test:"
	@sudo $(TEST_PERF) || true
	@echo ""
	@echo "4. IPC Test:"
	@sudo $(TEST_IPC) || true

# Limpiar archivos compilados
clean:
	@echo "Cleaning build files..."
	rm -rf $(OBJ_DIR) $(BIN_DIR)
	@cd $(KERNEL_DIR) && $(MAKE) clean 2>/dev/null || true
	@echo "✓ Clean complete"

# Limpiar todo incluyendo bases de datos y logs
distclean: clean
	@echo "Deep cleaning..."
	sudo rm -rf /var/lib/storage_mgr/*.db
	sudo rm -rf /var/run/storage_mgr.sock
	@echo "✓ Deep clean complete"

# Ayuda
help:
	@echo "Storage Manager Build System - Parts 6-10"
	@echo ""
	@echo "Available targets:"
	@echo "  all         - Build all components (default)"
	@echo "  daemon      - Build daemon only"
	@echo "  cli         - Build CLI client only"
	@echo "  kernel      - Build kernel module"
	@echo "  test        - Build and run tests"
	@echo "  install     - Install system-wide (requires sudo)"
	@echo "  clean       - Remove build files"
	@echo "  distclean   - Remove build files and data"
	@echo "  help        - Show this help"
	@echo ""
	@echo "Examples:"
	@echo "  make              # Build everything"
	@echo "  make test         # Run tests"
	@echo "  make kernel       # Build kernel module"
	@echo "  sudo make install # Install system-wide"

# Dependencias de headers
$(OBJ_DIR)/monitor.o: $(INC_DIR)/monitor.h
$(OBJ_DIR)/backup_engine.o: $(INC_DIR)/backup_engine.h
$(OBJ_DIR)/performance_tuner.o: $(INC_DIR)/performance_tuner.h
$(OBJ_DIR)/ipc_server.o: $(INC_DIR)/ipc_server.h
