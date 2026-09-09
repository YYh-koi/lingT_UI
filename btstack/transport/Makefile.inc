# BTstack common transport source and include list. Platform UART drivers are
# selected by their owning port and must not be added here.
# This tree exports no legacy block-UART platform factories; an owning port
# declares any such factory in its private header beside its implementation.
#
# Include this file from an application makefile together with src/Makefile.inc.
# Transport implementation files are collected only when present in this tree.

BTSTACK_ROOT ?= $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)

BTSTACK_TRANSPORT_INCLUDE_DIRS := \
    $(BTSTACK_ROOT)/transport \
    $(BTSTACK_ROOT)/include/adapter

BTSTACK_TRANSPORT_HEADERS := \
    $(BTSTACK_ROOT)/transport/btstack_slip.h \
    $(BTSTACK_ROOT)/transport/btstack_uart.h \
    $(BTSTACK_ROOT)/transport/btstack_uart_block.h \
    $(BTSTACK_ROOT)/transport/btstack_uart_slip_wrapper.h \
    $(BTSTACK_ROOT)/transport/hci_transport.h \
    $(BTSTACK_ROOT)/transport/hci_transport_em9304_spi.h \
    $(BTSTACK_ROOT)/transport/hci_transport_h4.h \
    $(BTSTACK_ROOT)/transport/hci_transport_h5.h \
    $(BTSTACK_ROOT)/transport/hci_transport_usb.h

BTSTACK_TRANSPORT_SOURCES := \
    $(wildcard $(BTSTACK_ROOT)/transport/btstack_slip.c) \
    $(wildcard $(BTSTACK_ROOT)/transport/btstack_uart_slip_wrapper.c) \
    $(wildcard $(BTSTACK_ROOT)/transport/hci_transport_em9304_spi.c) \
    $(wildcard $(BTSTACK_ROOT)/transport/hci_transport_h4.c) \
    $(wildcard $(BTSTACK_ROOT)/transport/hci_transport_h5.c) \
    $(wildcard $(BTSTACK_ROOT)/transport/hci_transport_usb.c)
