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
SCRIPT_DIR = scripts
SYSTEMD_DIR = systemd

# Crear directorios básicos (para segundo Makefile)
$(shell mkdir -p $(OBJ_DIR) $(BIN_DIR))

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

# =======================
#   DAEMON / SECURITY (segundo Makefile)
# =======================
DAEMON_SRC      = $(SRC_DIR)/daemon.c
DAEMON_MAIN_SRC = $(SRC_DIR)/daemon_main.c
DAEMON_OBJ      = $(OBJ_DIR)/daemon.o
DAEMON_MAIN_OBJ = $(OBJ_DIR)/daemon_main.o

# Reutilizamos nombre de binario del primer Makefile
DAEMON          = $(BIN_DIR)/storage_daemon
DAEMON_BIN      = $(DAEMON)

SECURITY_SRC    = $(SRC_DIR)/security_manager.c
SECURITY_OBJ    = $(OBJ_DIR)/security_manager.o

# Tests adicionales
TEST_DAEMON_SRC    = $(TEST_DIR)/test_daemon.c
TEST_DAEMON_BIN    = $(BIN_DIR)/test_daemon
TEST_SECURITY_SRC  = $(TEST_DIR)/test_security.c
TEST_SECURITY_BIN  = $(BIN_DIR)/test_security

# Ejecutables existentes
CLI          = $(BIN_DIR)/storage_cli
TEST_MONITOR = $(BIN_DIR)/test_monitor
TEST_BACKUP  = $(BIN_DIR)/test_backup
TEST_PERF    = $(BIN_DIR)/test_perf
TEST_IPC     = $(BIN_DIR)/test_ipc

# Archivos de instalación (segundo Makefile)
INSTALL_BIN      = /usr/local/bin
INSTALL_SYSTEMD  = /etc/systemd/system

SCRIPTS = $(SCRIPT_DIR)/health_check.sh \
          $(SCRIPT_DIR)/auto_backup.sh \
          $(SCRIPT_DIR)/perf_report.sh

SYSTEMD_FILES = $(SYSTEMD_DIR)/storage_daemon.service \
                $(SYSTEMD_DIR)/storage_backup.timer \
                $(SYSTEMD_DIR)/storage_backup.service \
                $(SYSTEMD_DIR)/storage_health.timer \
                $(SYSTEMD_DIR)/storage_health.service

# =======================
#   TARGETS PRINCIPALES
# =======================
.PHONY: all clean distclean test \
        dirs dirs-core dirs-extra \
        setup-loops check-loops \
        kernel install help info \
        run install-scripts install-systemd \
        uninstall-scripts uninstall-systemd \
        test-scripts clean-logs \
        install-automation uninstall-automation \
        install-all

all: dirs-core dirs-extra \
     $(TEST_PROG) \
     $(DAEMON) $(CLI) \
     $(TEST_MONITOR) $(TEST_BACKUP) $(TEST_PERF) $(TEST_IPC) \
     $(TEST_DAEMON_BIN) $(TEST_SECURITY_BIN)
	@echo "========================================="
	@echo "  Compilación exitosa"
	@echo "========================================="
	@echo "Ejecutables:"
	@echo "  - $(DAEMON)           (daemon principal)"
	@echo "  - $(CLI)              (cliente CLI)"
	@echo "  - $(TEST_PROG)        (tests core)"
	@echo "  - $(TEST_DAEMON_BIN)  (pruebas daemon)"
	@echo "  - $(TEST_SECURITY_BIN)(pruebas seguridad)"
	@echo ""

# =======================
#   PARTES 1-5
# =======================

setup-loops:
	@echo "Configurando loop devices..."
	@if [ $$(id -u) -ne 0 ]; then \
		echo "⚠ Se requieren permisos de root"; \
		echo "Ejecuta: sudo make setup-loops"; \
		exit 1; \
	fi
	@bash scripts/setup_loops.sh

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

dirs-core:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p /tmp/storage_test 2>/dev/null || true

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(wildcard $(INC_DIR)/*.h)
	@echo "Compilando $<..."
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_PROG): $(OBJECTS_CORE) $(TEST_DIR)/test_storage.c
	@echo "Enlazando programa de pruebas (storage_test)..."
	$(CC) $(CFLAGS) $(OBJECTS_CORE) $(TEST_DIR)/test_storage.c -o $@ $(LDFLAGS)
	@echo "✓ Compilación exitosa: $(TEST_PROG)"

test-core: $(TEST_PROG)
	@echo "========================================="
	@echo "EJECUTANDO PRUEBAS CORE"
	@echo "========================================="
	@if [ $$(id -u) -ne 0 ]; then \
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

dirs-extra:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)
	@mkdir -p /var/lib/storage_mgr 2>/dev/null || true
	@mkdir -p /backup 2>/dev/null || true

dirs: dirs-core dirs-extra

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Daemon (versión ampliada: usa objetos del segundo Makefile + extras)
$(DAEMON): $(DAEMON_OBJ) $(DAEMON_MAIN_OBJ) $(OBJECTS_EXTRA)
	@echo "Building daemon..."
	$(CC) $(CFLAGS) $(DAEMON_OBJ) $(DAEMON_MAIN_OBJ) $(OBJECTS_EXTRA) -o $@ $(LDFLAGS)
	@echo "✓ Daemon built: $@"

# Cliente CLI
$(CLI): $(OBJECTS_EXTRA) cli/storage_cli.c
	@echo "Building CLI client..."
	$(CC) $(CFLAGS) cli/storage_cli.c $(OBJECTS_EXTRA) -o $@ $(LDFLAGS)
	@echo "✓ CLI built: $@"

# Tests individuales (monitor/backup/perf/ipc)
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

# =======================
#   REGLAS DAEMON/SECURITY EXTRA
# =======================

$(DAEMON_OBJ): $(DAEMON_SRC) $(INC_DIR)/daemon.h
	@echo "Compilando daemon.c..."
	$(CC) $(CFLAGS) -c $< -o $@

$(DAEMON_MAIN_OBJ): $(DAEMON_MAIN_SRC) $(INC_DIR)/daemon.h
	@echo "Compilando daemon_main.c..."
	$(CC) $(CFLAGS) -c $< -o $@

$(SECURITY_OBJ): $(SECURITY_SRC) $(INC_DIR)/security_manager.h
	@echo "Compilando security_manager.c..."
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_DAEMON_BIN): $(TEST_DAEMON_SRC) $(DAEMON_OBJ)
	@echo "Compilando test_daemon..."
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(TEST_SECURITY_BIN): $(TEST_SECURITY_SRC) $(SECURITY_OBJ)
	@echo "Compilando test_security..."
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# =======================
#   KERNEL / INSTALACIÓN
# =======================

kernel:
	@echo "Building kernel module..."
	@cd $(KERNEL_DIR) && $(MAKE)
	@echo "✓ Kernel module built"

install: all kernel
	@echo "Installing storage manager..."
	sudo cp $(DAEMON) $(INSTALL_BIN)/
	sudo cp $(CLI) $(INSTALL_BIN)/
	sudo mkdir -p /var/lib/storage_mgr
	sudo mkdir -p /backup
	sudo chmod +x $(INSTALL_BIN)/storage_daemon
	@echo "✓ Installation complete"
	@echo ""
	@echo "To load kernel module:"
	@echo "  cd $(KERNEL_DIR) && sudo make install"

run: $(DAEMON)
	@echo "Iniciando daemon..."
	sudo ./$(DAEMON)

# =======================
#   TESTS COMPLETOS
# =======================

test: test-core $(TEST_MONITOR) $(TEST_BACKUP) $(TEST_PERF) $(TEST_IPC) $(TEST_DAEMON_BIN) $(TEST_SECURITY_BIN)
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
	@echo ""
	@echo "--- Pruebas del daemon ---"
	./$(TEST_DAEMON_BIN)
	@echo ""
	@echo "--- Pruebas de seguridad ---"
	./$(TEST_SECURITY_BIN)
	@echo ""
	@echo "Todas las pruebas completadas."

# =======================
#   AUTOMATION & SYSTEMD
# =======================

install-scripts: $(SCRIPTS)
	@echo "Instalando scripts de automatización..."
	sudo install -m 755 $(SCRIPT_DIR)/health_check.sh $(INSTALL_BIN)/
	sudo install -m 755 $(SCRIPT_DIR)/auto_backup.sh $(INSTALL_BIN)/
	sudo install -m 755 $(SCRIPT_DIR)/perf_report.sh $(INSTALL_BIN)/
	@echo "Scripts instalados en $(INSTALL_BIN)/"

install-systemd: $(SYSTEMD_FILES)
	@echo "Instalando archivos systemd..."
	sudo install -m 644 $(SYSTEMD_DIR)/*.service $(INSTALL_SYSTEMD)/
	sudo install -m 644 $(SYSTEMD_DIR)/*.timer $(INSTALL_SYSTEMD)/
	sudo systemctl daemon-reload
	@echo "Archivos systemd instalados."
	@echo ""
	@echo "Para habilitar los servicios:"
	@echo "  sudo systemctl enable storage_daemon.service"
	@echo "  sudo systemctl enable storage_backup.timer"
	@echo "  sudo systemctl enable storage_health.timer"
	@echo ""
	@echo "Para iniciar los servicios:"
	@echo "  sudo systemctl start storage_daemon.service"
	@echo "  sudo systemctl start storage_backup.timer"
	@echo "  sudo systemctl start storage_health.timer"

uninstall-scripts:
	@echo "Desinstalando scripts..."
	sudo rm -f $(INSTALL_BIN)/health_check.sh
	sudo rm -f $(INSTALL_BIN)/auto_backup.sh
	sudo rm -f $(INSTALL_BIN)/perf_report.sh
	@echo "Scripts desinstalados."

uninstall-systemd:
	@echo "Desinstalando servicios systemd..."
	sudo systemctl stop storage_backup.timer 2>/dev/null || true
	sudo systemctl stop storage_health.timer 2>/dev/null || true
	sudo systemctl disable storage_backup.timer 2>/dev/null || true
	sudo systemctl disable storage_health.timer 2>/dev/null || true
	sudo rm -f $(INSTALL_SYSTEMD)/storage_daemon.service
	sudo rm -f $(INSTALL_SYSTEMD)/storage_backup.timer
	sudo rm -f $(INSTALL_SYSTEMD)/storage_backup.service
	sudo rm -f $(INSTALL_SYSTEMD)/storage_health.timer
	sudo rm -f $(INSTALL_SYSTEMD)/storage_health.service
	sudo systemctl daemon-reload
	@echo "Servicios systemd desinstalados."

test-scripts:
	@echo "========================================="
	@echo "  Probando scripts de automatización"
	@echo "========================================="
	@echo ""
	@echo "--- Test: health_check.sh ---"
	sudo $(SCRIPT_DIR)/health_check.sh || true
	@echo ""
	@echo "--- Test: auto_backup.sh (dry-run) ---"
	sudo $(SCRIPT_DIR)/auto_backup.sh --dry-run
	@echo ""
	@echo "--- Test: perf_report.sh ---"
	sudo $(SCRIPT_DIR)/perf_report.sh --output /tmp/test_perf_report.txt
	@echo ""
	@echo "Contenido del reporte (primeras 30 líneas):"
	@head -30 /tmp/test_perf_report.txt || true
	@echo ""
	@echo "Pruebas de scripts completadas."

clean-logs:
	@echo "Limpiando logs de automatización..."
	sudo rm -f /var/log/storage_health.log
	sudo rm -f /var/log/storage_backup.log
	sudo rm -f /var/log/storage_health_report_*.txt
	sudo rm -f /var/log/performance_report_*.html
	sudo rm -f /var/log/performance_report_*.txt
	@echo "Logs limpiados."

install-automation: install-scripts install-systemd
	@echo ""
	@echo "========================================="
	@echo "  Automatización instalada exitosamente"
	@echo "========================================="
	@echo ""
	@echo "Scripts disponibles:"
	@echo "  - health_check.sh   (verificación de salud)"
	@echo "  - auto_backup.sh    (backup automatizado)"
	@echo "  - perf_report.sh    (reporte de rendimiento)"
	@echo ""
	@echo "Servicios systemd instalados:"
	@echo "  - storage_daemon.service"
	@echo "  - storage_backup.timer"
	@echo "  - storage_health.timer"

uninstall-automation: uninstall-systemd uninstall-scripts
	@echo "Automatización desinstalada completamente."

install-all: install install-automation
	@echo ""
	@echo "========================================="
	@echo "  Instalación completa finalizada"
	@echo "========================================="
	
# =======================
#   LIMPIEZA / INFO / HELP
# =======================

clean:
	@echo "Cleaning build files..."
	rm -rf $(BUILD_DIR) $(OBJ_DIR) $(BIN_DIR)
	@cd $(KERNEL_DIR) && $(MAKE) clean 2>/dev/null || true
	rm -f *.pid
	rm -f test_daemon.pid storage_mgr.pid
	rm -f audit.log
	rm -f /tmp/test_*.txt
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
	@echo "  all                  - Compila todo (core + daemon/CLI/tests)"
	@echo "  test-core            - Ejecuta tests de RAID/LVM/filesystem/memory/security"
	@echo "  test                 - Tests core + monitor/backup/perf/ipc + daemon/security"
	@echo "  setup-loops          - Configura loop devices (requiere sudo)"
	@echo "  check-loops          - Verifica loop devices"
	@echo "  kernel               - Compila módulo del kernel"
	@echo "  run                  - Ejecuta daemon"
	@echo "  install              - Instala daemon y CLI (requiere sudo)"
	@echo "  install-scripts      - Instala scripts de automatización"
	@echo "  install-systemd      - Instala servicios systemd"
	@echo "  install-automation   - Scripts + systemd"
	@echo "  install-all          - Instalación completa"
	@echo "  clean                - Limpia binarios y objetos"
	@echo "  distclean            - Limpieza profunda (incluye RAID, loop devices, datos)"
	@echo "  test-scripts         - Probar scripts de automatización"
	@echo "  clean-logs           - Limpiar logs de automatización"
	@echo "  uninstall-scripts    - Desinstalar scripts"
	@echo "  uninstall-systemd    - Desinstalar systemd"
	@echo "  uninstall-automation - Desinstalar automatización"
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
