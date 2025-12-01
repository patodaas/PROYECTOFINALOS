CC      = gcc
CFLAGS  = -Wall -Wextra -g -pthread -I./include
LDFLAGS = -lpthread -lsqlite3 -lssl -lcrypto -lm

# Directorios
SRC_DIR    = src
INC_DIR    = include
TEST_DIR   = tests
BUILD_DIR  = build
OBJ_DIR    = obj
BIN_DIR    = bin
KERNEL_DIR = kernel_module
SCRIPT_DIR = scripts
SYSTEMD_DIR = systemd

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

# Daemon y CLI
DAEMON_SRC      = $(SRC_DIR)/daemon.c
DAEMON_MAIN_SRC = $(SRC_DIR)/daemon_main.c
DAEMON_OBJ      = $(OBJ_DIR)/daemon.o
DAEMON_MAIN_OBJ = $(OBJ_DIR)/daemon_main.o

DAEMON = $(BIN_DIR)/storage_daemon
CLI    = $(BIN_DIR)/storage_cli

# Tests adicionales
TEST_DAEMON_BIN    = $(BIN_DIR)/test_daemon
TEST_SECURITY_BIN  = $(BIN_DIR)/test_security
TEST_MONITOR       = $(BIN_DIR)/test_monitor
TEST_BACKUP        = $(BIN_DIR)/test_backup
TEST_PERF          = $(BIN_DIR)/test_perf
TEST_IPC           = $(BIN_DIR)/test_ipc

# Instalación
INSTALL_BIN     = /usr/local/bin
INSTALL_SYSTEMD = /etc/systemd/system

# =======================
#   TARGETS PRINCIPALES
# =======================
.PHONY: all clean distclean test test-core \
        dirs dirs-core dirs-extra \
        setup-loops check-loops \
        kernel install help info run \
        install-scripts install-systemd \
        uninstall-scripts uninstall-systemd \
        test-scripts clean-logs \
        install-automation uninstall-automation \
        install-all

all: dirs $(TEST_PROG) $(DAEMON) $(CLI) $(TEST_MONITOR) $(TEST_BACKUP) $(TEST_PERF) $(TEST_IPC) $(TEST_DAEMON_BIN) $(TEST_SECURITY_BIN)
	@echo "========================================="
	@echo "  ✓ Compilación exitosa"
	@echo "========================================="
	@echo "Ejecutables disponibles:"
	@echo "  - $(TEST_PROG)        (tests partes 1-5)"
	@echo "  - $(DAEMON)           (daemon)"
	@echo "  - $(CLI)              (cliente CLI)"
	@echo "  - $(TEST_MONITOR)     (test monitoring)"
	@echo "  - $(TEST_BACKUP)      (test backup)"
	@echo "  - $(TEST_PERF)        (test performance)"
	@echo "  - $(TEST_IPC)         (test IPC)"
	@echo "  - $(TEST_DAEMON_BIN)  (test daemon)"
	@echo "  - $(TEST_SECURITY_BIN)(test security)"
	@echo ""

# =======================
#   DIRECTORIOS
# =======================
dirs: dirs-core dirs-extra

dirs-core:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p /tmp/storage_test 2>/dev/null || true

dirs-extra:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)
	@mkdir -p /var/lib/storage_mgr 2>/dev/null || true
	@mkdir -p /backup 2>/dev/null || true

# =======================
#   PARTES 1-5 (CORE)
# =======================
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(wildcard $(INC_DIR)/*.h)
	@echo "Compilando $<..."
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_PROG): dirs-core $(OBJECTS_CORE) $(TEST_DIR)/test_storage.c
	@echo "Enlazando $(TEST_PROG)..."
	$(CC) $(CFLAGS) $(OBJECTS_CORE) $(TEST_DIR)/test_storage.c -o $@ $(LDFLAGS)
	@echo "✓ Compilado: $(TEST_PROG)"

test-core: $(TEST_PROG)
	@echo "========================================="
	@echo "EJECUTANDO PRUEBAS PARTES 1-5"
	@echo "========================================="
	@if [ $$(id -u) -ne 0 ]; then \
		echo "⚠ Las pruebas requieren root"; \
		echo "Ejecuta: sudo make test-core"; \
		exit 1; \
	fi
	@$(TEST_PROG)

setup-loops:
	@echo "Configurando loop devices..."
	@if [ $$(id -u) -ne 0 ]; then \
		echo "⚠ Requiere root"; \
		echo "Ejecuta: sudo make setup-loops"; \
		exit 1; \
	fi
	@if [ -f scripts/setup_loops.sh ]; then \
		bash scripts/setup_loops.sh; \
	else \
		echo "⚠ scripts/setup_loops.sh no encontrado"; \
	fi

check-loops:
	@echo "========================================="
	@echo "Loop devices disponibles"
	@echo "========================================="
	@losetup -a | grep storage_test || echo "Ninguno configurado"

# =======================
#   PARTES 6-10 (EXTRA)
# =======================
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(wildcard $(INC_DIR)/*.h)
	@echo "Compilando $<..."
	$(CC) $(CFLAGS) -c $< -o $@

$(DAEMON): dirs-extra $(DAEMON_OBJ) $(DAEMON_MAIN_OBJ) $(OBJECTS_EXTRA)
	@echo "Enlazando daemon..."
	$(CC) $(CFLAGS) $(DAEMON_OBJ) $(DAEMON_MAIN_OBJ) $(OBJECTS_EXTRA) -o $@ $(LDFLAGS)
	@echo "✓ Daemon compilado: $@"

$(CLI): dirs-extra $(OBJECTS_EXTRA) cli/storage_cli.c
	@echo "Enlazando CLI..."
	$(CC) $(CFLAGS) cli/storage_cli.c $(OBJECTS_EXTRA) -o $@ $(LDFLAGS)
	@echo "✓ CLI compilado: $@"

$(TEST_MONITOR): dirs-extra $(OBJ_DIR)/monitor.o $(OBJ_DIR)/utils.o tests/test_monitor.c
	@echo "Compilando test_monitor..."
	$(CC) $(CFLAGS) tests/test_monitor.c $(OBJ_DIR)/monitor.o $(OBJ_DIR)/utils.o -o $@ $(LDFLAGS)

$(TEST_BACKUP): dirs-extra $(OBJ_DIR)/backup_engine.o $(OBJ_DIR)/utils.o tests/test_backup.c
	@echo "Compilando test_backup..."
	$(CC) $(CFLAGS) tests/test_backup.c $(OBJ_DIR)/backup_engine.o $(OBJ_DIR)/utils.o -o $@ $(LDFLAGS)

$(TEST_PERF): dirs-extra $(OBJ_DIR)/performance_tuner.o $(OBJ_DIR)/utils.o tests/test_perf.c
	@echo "Compilando test_perf..."
	$(CC) $(CFLAGS) tests/test_perf.c $(OBJ_DIR)/performance_tuner.o $(OBJ_DIR)/utils.o -o $@ $(LDFLAGS)

$(TEST_IPC): dirs-extra $(OBJ_DIR)/ipc_server.o $(OBJ_DIR)/utils.o tests/test_ipc.c
	@echo "Compilando test_ipc..."
	$(CC) $(CFLAGS) tests/test_ipc.c $(OBJ_DIR)/ipc_server.o $(OBJ_DIR)/utils.o -o $@ $(LDFLAGS)

$(TEST_DAEMON_BIN): dirs-extra $(DAEMON_OBJ) tests/test_daemon.c
	@echo "Compilando test_daemon..."
	$(CC) $(CFLAGS) tests/test_daemon.c $(DAEMON_OBJ) -o $@ $(LDFLAGS)

$(TEST_SECURITY_BIN): dirs-extra $(OBJ_DIR)/security_manager.o tests/test_security.c
	@echo "Compilando test_security..."
	$(CC) $(CFLAGS) tests/test_security.c $(OBJ_DIR)/security_manager.o -o $@ $(LDFLAGS)

# =======================
#   KERNEL MODULE
# =======================
kernel:
	@echo "Compilando módulo del kernel..."
	@if [ -d $(KERNEL_DIR) ]; then \
		cd $(KERNEL_DIR) && $(MAKE); \
		echo "✓ Módulo compilado"; \
	else \
		echo "⚠ Directorio $(KERNEL_DIR) no encontrado"; \
	fi

# =======================
#   TESTS COMPLETOS
# =======================
test: test-core
	@echo ""
	@echo "========================================="
	@echo "EJECUTANDO TESTS ADICIONALES"
	@echo "========================================="
	@if [ -f $(TEST_MONITOR) ]; then \
		echo "Test Monitor:"; \
		sudo $(TEST_MONITOR) || true; \
	fi
	@if [ -f $(TEST_BACKUP) ]; then \
		echo "Test Backup:"; \
		sudo $(TEST_BACKUP) || true; \
	fi
	@if [ -f $(TEST_PERF) ]; then \
		echo "Test Performance:"; \
		sudo $(TEST_PERF) || true; \
	fi
	@if [ -f $(TEST_IPC) ]; then \
		echo "Test IPC:"; \
		sudo $(TEST_IPC) || true; \
	fi
	@if [ -f $(TEST_DAEMON_BIN) ]; then \
		echo "Test Daemon:"; \
		$(TEST_DAEMON_BIN) || true; \
	fi
	@if [ -f $(TEST_SECURITY_BIN) ]; then \
		echo "Test Security:"; \
		$(TEST_SECURITY_BIN) || true; \
	fi

# =======================
#   INSTALACIÓN
# =======================
install: all kernel
	@echo "Instalando..."
	@if [ $$(id -u) -ne 0 ]; then \
		echo "⚠ Requiere root. Ejecuta: sudo make install"; \
		exit 1; \
	fi
	install -m 755 $(DAEMON) $(INSTALL_BIN)/
	install -m 755 $(CLI) $(INSTALL_BIN)/
	mkdir -p /var/lib/storage_mgr
	mkdir -p /backup
	@echo "✓ Instalación completa"

install-scripts:
	@echo "Instalando scripts..."
	@if [ $$(id -u) -ne 0 ]; then \
		echo "⚠ Requiere root"; \
		exit 1; \
	fi
	@if [ -d $(SCRIPT_DIR) ]; then \
		install -m 755 $(SCRIPT_DIR)/*.sh $(INSTALL_BIN)/ 2>/dev/null || true; \
	fi
	@echo "✓ Scripts instalados"

install-systemd:
	@echo "Instalando servicios systemd..."
	@if [ $$(id -u) -ne 0 ]; then \
		echo "⚠ Requiere root"; \
		exit 1; \
	fi
	@if [ -d $(SYSTEMD_DIR) ]; then \
		install -m 644 $(SYSTEMD_DIR)/*.service $(INSTALL_SYSTEMD)/ 2>/dev/null || true; \
		install -m 644 $(SYSTEMD_DIR)/*.timer $(INSTALL_SYSTEMD)/ 2>/dev/null || true; \
		systemctl daemon-reload; \
	fi
	@echo "✓ Systemd instalado"

install-automation: install-scripts install-systemd
	@echo "✓ Automatización instalada"

install-all: install install-automation
	@echo "✓ Instalación completa finalizada"

# =======================
#   DESINSTALACIÓN
# =======================
uninstall-scripts:
	@echo "Desinstalando scripts..."
	@if [ $$(id -u) -ne 0 ]; then \
		echo "⚠ Requiere root"; \
		exit 1; \
	fi
	rm -f $(INSTALL_BIN)/health_check.sh
	rm -f $(INSTALL_BIN)/auto_backup.sh
	rm -f $(INSTALL_BIN)/perf_report.sh

uninstall-systemd:
	@echo "Desinstalando systemd..."
	@if [ $$(id -u) -ne 0 ]; then \
		echo "⚠ Requiere root"; \
		exit 1; \
	fi
	systemctl stop storage_*.timer 2>/dev/null || true
	systemctl disable storage_*.timer 2>/dev/null || true
	rm -f $(INSTALL_SYSTEMD)/storage_*.service
	rm -f $(INSTALL_SYSTEMD)/storage_*.timer
	systemctl daemon-reload

uninstall-automation: uninstall-systemd uninstall-scripts
	@echo "✓ Automatización desinstalada"

# =======================
#   EJECUCIÓN
# =======================
run: $(DAEMON)
	@echo "Iniciando daemon..."
	@if [ $$(id -u) -ne 0 ]; then \
		echo "⚠ Requiere root"; \
		exit 1; \
	fi
	sudo ./$(DAEMON)

# =======================
#   LIMPIEZA
# =======================
clean:
	@echo "Limpiando..."
	rm -rf $(BUILD_DIR) $(OBJ_DIR) $(BIN_DIR)
	@if [ -d $(KERNEL_DIR) ]; then \
		cd $(KERNEL_DIR) && $(MAKE) clean 2>/dev/null || true; \
	fi
	rm -f *.pid *.log
	@echo "✓ Limpieza completa"

distclean: clean
	@echo "Limpieza profunda..."
	@if [ $$(id -u) -eq 0 ]; then \
		mdadm --stop /dev/md* 2>/dev/null || true; \
		losetup -D 2>/dev/null || true; \
		rm -rf /var/lib/storage_mgr 2>/dev/null || true; \
		rm -f /var/run/storage_mgr.sock 2>/dev/null || true; \
	fi
	rm -rf /tmp/storage_test 2>/dev/null || true
	@echo "✓ Limpieza profunda completa"

clean-logs:
	@echo "Limpiando logs..."
	@if [ $$(id -u) -eq 0 ]; then \
		rm -f /var/log/storage_*.log; \
	fi
	@echo "✓ Logs limpiados"

# =======================
#   TEST SCRIPTS
# =======================
test-scripts:
	@echo "========================================="
	@echo "Probando scripts"
	@echo "========================================="
	@if [ -f $(SCRIPT_DIR)/health_check.sh ]; then \
		echo "Test health_check:"; \
		sudo $(SCRIPT_DIR)/health_check.sh || true; \
	fi
	@if [ -f $(SCRIPT_DIR)/auto_backup.sh ]; then \
		echo "Test auto_backup (dry-run):"; \
		sudo $(SCRIPT_DIR)/auto_backup.sh --dry-run || true; \
	fi

# =======================
#   INFO Y AYUDA
# =======================
info:
	@echo "========================================="
	@echo "Información del Sistema"
	@echo "========================================="
	@echo "Compilador: $(CC)"
	@echo "Flags: $(CFLAGS)"
	@echo ""
	@echo "Loop devices:"
	@losetup -a 2>/dev/null || echo "  Ninguno"
	@echo ""
	@echo "RAID activos:"
	@cat /proc/mdstat 2>/dev/null | grep "^md" || echo "  Ninguno"
	@echo ""
	@echo "Volúmenes LVM:"
	@sudo lvs 2>/dev/null || echo "  Ninguno"
	@echo "========================================="

help:
	@echo "========================================="
	@echo "Enterprise Storage Manager - Makefile"
	@echo "========================================="
	@echo "Targets principales:"
	@echo "  all              - Compila todo"
	@echo "  test-core        - Ejecuta tests partes 1-5"
	@echo "  test             - Ejecuta todos los tests"
	@echo "  setup-loops      - Configura loop devices (sudo)"
	@echo "  check-loops      - Verifica loop devices"
	@echo "  kernel           - Compila módulo kernel"
	@echo "  run              - Ejecuta daemon (sudo)"
	@echo "  install          - Instala binarios (sudo)"
	@echo "  install-all      - Instalación completa (sudo)"
	@echo "  clean            - Limpia binarios"
	@echo "  distclean        - Limpieza profunda (sudo)"
	@echo "  help             - Muestra esta ayuda"
	@echo "  info             - Info del sistema"
	@echo "========================================="
