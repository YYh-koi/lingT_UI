# Makefile to collect all C source files of src

BTSTACK_ROOT ?= $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)

SRC_INCLUDE_DIRS := \
    $(BTSTACK_ROOT)/src \
    $(BTSTACK_ROOT)/transport \
    $(BTSTACK_ROOT)/include/adapter

SRC_FILES = \
    ad_parser.c \
    btstack_audio.c \
    btstack_base64_decoder.c \
    btstack_crypto.c \
    btstack_hid_parser.c \
    btstack_linked_list.c \
    btstack_memory.c \
    btstack_memory_pool.c \
    btstack_ring_buffer.c \
    btstack_run_loop.c \
    btstack_tlv.c \
    btstack_util.c \
    hci.c \
    hci_cmd.c \
    hci_dump.c \
    l2cap.c \
    l2cap_signaling.c \
