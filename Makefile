# Makefile unificado para Enterprise Storage Manager (Partes 1-10)

CC      = gcc
CFLAGS  = -Wall -Wextra -g -pthread -I./include
LDFLAGS = -lpthread -lsqlite3 -lssl -lcrypto -lm

# Directorios
SRC_DIR    = src
INC_DIR    = include
TEST_DIR   = tests
BUILD_DIR  = build        # para storage_test (partes 1-5)
OBJ_DIR    = obj          # para daemon/CLI/tests (partes 6-10)
BIN_DIR    = bin
KERNEL_DIR = kernel_module

# =======================
#   FUENTES PARTES 1-5
# =======================
SOURCES_CORE = \
	$(SRC_DIR)/utils.c \
	$(SRC_DIR)/raid_manager.c \
	$(SRC_DIR)/lvm_manager.c \
	$(SRC_DIR)/filesystem_ops.c \
	$(SRC_DIR)/memory_manager.c \
	$(SRC_DIR)/security_manager.c

OBJECTS_CORE = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES_CORE))

TEST_PROG = $(BUILD_DIR)/storage_test

# =======================
#   FUENTES PARTES 6-10
# =======================
SOURCES_EXTRA = \
	$(SRC_DIR)/monitor.c \
	$(SRC_DIR)/backup_engine.c \
	$(SRC_DIR)/performance_tuner.c \
	$(SRC_DIR)/ipc_server.c \
	$(SRC_DIR)/utils.c

OBJECTS_EXTRA = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCES_EXTRA))

# Ejecutables
DAEMON       = $(BIN_DIR)/storage_daemon
CLI          = $(BIN_DIR)/storage_cli
TEST_MONITOR = $(BIN_DIR)/test_monitor
TEST_BACKUP  = $(BIN_DIR)/test_backup
TEST_PERF    = $(BIN_DIR)/test_perf
TEST_IPC     = $(BIN_DIR)/test_ipc

# =======================
#   TARGETS PRINCIPALES
# =======================
.PHONY: all clean distclean test \
        dirs dirs-core dirs-extra \
        setup-loops check-loops \
        kernel install help info

all: dirs-core dirs-extra $(TEST_PROG) $(DAEMON) $(CLI) $(TEST_MONITOR) $(TEST_BACKUP) $(TEST_PERF) $(TEST_IPC)

# =======================
#   PARTES 1-5
# =======================

# Configurar loop devices
setup-loops:
	@echo "Configurando loop devices..."
	@if [ $$\(id -u\) -ne 0 ]; then \
		echo "⚠ Se requieren permisos de root"; \
		echo "Ejecuta: sudo make setup-loops"; \
		exit 1; \
	fi
	@bash scripts/setup_loops.sh

# Verificar loop devices
check-loops:
	@echo "========================================="
	@echo "Verificando loop devices"
	@echo "========================================="
	@losetup -a | grep storage_test || echo "No hay loop devices configurados"
	@echo ""
	@echo "Dispositivos disponibles:"
	@for i in 0 1 2 3 4 5; do \
		if [ -b /dev/loop$$i ]; then \
			SIZE=$$(blockdev --getsize64 /dev/loop$$i 2>/dev/null || echo "0"); \
			if [ "$$SIZE" -gt 0 ]; then \
				echo "  ✓ /dev/loop$$i"; \
			fi; \
		fi; \
	done

# Crear directorios necesarios para partes 1-5
dirs-core:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p /tmp/storage_test 2>/dev/null || true

# Compilar archivos objeto core
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(wildcard $(INC_DIR)/*.h)
	@echo "Compilando $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Enlazar programa de pruebas core
$(TEST_PROG): $(OBJECTS_CORE) $(TEST_DIR)/test_storage.c
	@echo "Enlazando programa de pruebas (storage_test)..."
	$(CC) $(CFLAGS) $(OBJECTS_CORE) $(TEST_DIR)/test_storage.c -o $@ $(LDFLAGS)
	@echo "✓ Compilación exitosa: $(TEST_PROG)"

# Ejecutar pruebas core (requiere sudo)
test-core: $(TEST_PROG)
	@echo "========================================="
	@echo "EJECUTANDO PRUEBAS CORE"
	@echo "========================================="
	@if [ $$\(id -u\) -ne 0 ]; then \
		echo "⚠ Las pruebas requieren permisos de root"; \
		echo "Ejecuta: sudo make test-core"; \
		exit 1; \
	fi
	@echo ""
	@echo "Verificando loop devices..."
	@LOOPS=$$(losetup -a | grep storage_test | wc -l); \
	if [ $$LOOPS -lt 4 ]; then \
		echo "⚠ No hay suficientes loop devices configurados ($$LOOPS/6)"; \
		echo "Ejecutando setup automático..."; \
		echo ""; \
		$(MAKE) setup-loops; \
		echo ""; \
	fi
	@echo "Ejecutando tests core..."
	@$(TEST_PROG)

# =======================
#   PARTES 6-10
# =======================

# Crear directorios necesarios para partes 6-10
dirs-extra:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)
	@mkdir -p /var/lib/storage_mgr 2>/dev/null || true
	@mkdir -p /backup 2>/dev/null || true

# Alias genérico
dirs: dirs-core dirs-extra

# Regla para compilar .c a .o (extra)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Daemon principal
$(DAEMON): $(OBJECTS_EXTRA) $(SRC_DIR)/daemon_main.c
	@echo "Building daemon..."
	$(CC) $(CFLAGS) $(SRC_DIR)/daemon_main.c $(OBJECTS_EXTRA) -o $@ $(LDFLAGS)
	@echo "✓ Daemon built: $@"

# Cliente CLI
$(CLI): $(OBJECTS_EXTRA) cli/storage_cli.c
	@echo "Building CLI client..."
	$(CC) $(CFLAGS) cli/storage_cli.c $(OBJECTS_EXTRA) -o $@ $(LDFLAGS)
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

# Tests completos (core + extra)
test: test-core $(TEST_MONITOR) $(TEST_BACKUP) $(TEST_PERF) $(TEST_IPC)
	@echo ""
	@echo "=== Running Extra Tests ==="
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

# =======================
#   LIMPIEZA / INFO
# =======================

clean:
	@echo "Cleaning build files..."
	rm -rf $(BUILD_DIR) $(OBJ_DIR) $(BIN_DIR)
	@cd $(KERNEL_DIR) && $(MAKE) clean 2>/dev/null || true
	@echo "✓ Clean complete"

distclean: clean
	@echo "Deep cleaning..."
	sudo rm -rf /var/lib/storage_mgr/*.db 2>/dev/null || true
	sudo rm -rf /var/run/storage_mgr.sock 2>/dev/null || true
	@sudo mdadm --stop /dev/md* 2>/dev/null || true
	@sudo losetup -D 2>/dev/null || true
	@rm -rf /tmp/storage_test 2>/dev/null || true
	@echo "✓ Deep clean complete"

help:
	@echo "========================================="
	@echo "Enterprise Storage Manager - Makefile Unificado"
	@echo "========================================="
	@echo "Targets principales:"
	@echo "  all          - Compila todo (core + daemon/CLI/tests)"
	@echo "  test-core    - Ejecuta tests de RAID/LVM/filesystem/memory/security"
	@echo "  test         - Tests core + monitor/backup/perf/ipc"
	@echo "  setup-loops  - Configura loop devices (requiere sudo)"
	@echo "  check-loops  - Verifica loop devices"
	@echo "  kernel       - Compila módulo del kernel"
	@echo "  install      - Instala daemon y CLI (requiere sudo)"
	@echo "  clean        - Limpia binarios y objetos"
	@echo "  distclean    - Limpieza profunda (incluye RAID, loop devices, datos)"
	@echo "========================================="

info:
	@echo "========================================="
	@echo "Información del Sistema"
	@echo "========================================="
	@echo "Compilador: $(CC)"
	@echo "Flags: $(CFLAGS)"
	@echo "Loop devices disponibles:"
	@losetup -a || echo "  Ninguno configurado"
	@echo ""
	@echo "Arrays RAID activos:"
	@cat /proc/mdstat 2>/dev/null || echo "  Ninguno activo"
	@echo ""
	@echo "Volúmenes LVM:"
	@sudo lvs 2>/dev/null || echo "  Ninguno configurado"
	@echo "========================================="

