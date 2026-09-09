# GPCV2178 selected-port manifest.
# Include btstack/Makefile.inc first so BTSTACK_ROOT is defined.

BTSTACK_GPCV2178_INCLUDE_DIRS := \
    $(BTSTACK_ROOT)/ports/gpcv2178

BTSTACK_GPCV2178_PORT_SOURCES := \
    $(BTSTACK_ROOT)/ports/gpcv2178/btstack_gpcv2178_port.c \
    $(BTSTACK_ROOT)/ports/gpcv2178/btstack_platform_gpcv2178.c \
    $(BTSTACK_ROOT)/ports/gpcv2178/btstack_rtos_gpcv2178.c \
    $(BTSTACK_ROOT)/ports/gpcv2178/btstack_hci_dump_gpcv2178.c \
    $(BTSTACK_ROOT)/ports/gpcv2178/btstack_run_loop_gpcv2178.c \
    $(BTSTACK_ROOT)/ports/gpcv2178/btstack_uart_gpcv2178.c \
    $(BTSTACK_ROOT)/ports/gpcv2178/btstack_control_gpcv2178.c \
    $(BTSTACK_ROOT)/ports/gpcv2178/btstack_storage_gpcv2178.c
