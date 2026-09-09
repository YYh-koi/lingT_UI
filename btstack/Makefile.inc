# BTstack common source list.
#
# Include this file from an application makefile, or use it as the source of
# truth for another build system.

ifndef BTSTACK_ROOT
BTSTACK_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
endif

BTSTACK_INCLUDE_DIRS := \
    $(BTSTACK_ROOT)/include \
    $(BTSTACK_ROOT)/include/adapter \
    $(BTSTACK_ROOT)/include/port \
    $(BTSTACK_ROOT)/at \
    $(BTSTACK_ROOT)/src \
    $(BTSTACK_ROOT)/transport \
    $(BTSTACK_ROOT)/3rd-party/bluedroid/decoder/include \
    $(BTSTACK_ROOT)/3rd-party/md5 \
    $(BTSTACK_ROOT)/3rd-party/yxml

# Select one frontend at compile time as well as in the source list. These
# flags let the selected port omit the other frontend's static worker memory.
BTSTACK_API_BUILD_CPPFLAGS := -DBTSTACK_BUILD_VARIANT_API=1
BTSTACK_AT_BUILD_CPPFLAGS := -DBTSTACK_BUILD_VARIANT_AT=1

BTSTACK_AT_CORE_SOURCES := \
    $(BTSTACK_ROOT)/at/btstack_at.c \
    $(BTSTACK_ROOT)/at/btstack_at_command_table.c \
    $(BTSTACK_ROOT)/at/btstack_at_dispatch_table.c \
    $(BTSTACK_ROOT)/at/btstack_at_parser.c \
    $(BTSTACK_ROOT)/at/btstack_at_protocol.c \
    $(BTSTACK_ROOT)/at/btstack_at_core_adapter.c

BTSTACK_AT_FRONTEND_SOURCES := \
    $(BTSTACK_AT_CORE_SOURCES) \
    $(BTSTACK_ROOT)/at/bt_xstack_at_frontend.c

# Deprecated compatibility alias for the internal/legacy AT core.
BTSTACK_AT_SOURCES := $(BTSTACK_AT_CORE_SOURCES)

BTSTACK_API_FRONTEND_SOURCES := \
    $(BTSTACK_ROOT)/api/bt_xstack_api_core.c

BTSTACK_ADAPTER_SOURCES := \
    $(BTSTACK_ROOT)/core/btstack_adapter.c \
    $(BTSTACK_ROOT)/core/btstack_call_audio_tx.c \
    $(BTSTACK_ROOT)/core/btstack_platform.c \
    $(BTSTACK_ROOT)/core/btstack_port_ops.c \
    $(BTSTACK_ROOT)/core/btstack_property.c \
    $(BTSTACK_ROOT)/core/btstack_ring.c

BTSTACK_EVENT_SOURCES := \
    $(BTSTACK_ROOT)/core/btstack_event_queue.c

BTSTACK_RUNTIME_SOURCES := \
    $(BTSTACK_ROOT)/core/btstack_core_dispatcher.c \
    $(BTSTACK_ROOT)/core/btstack_event_mapper.c \
    $(BTSTACK_ROOT)/core/btstack_runtime.c

BTSTACK_TRANSPORT_SOURCES := \
    $(BTSTACK_ROOT)/transport/hci_transport_h5.c \
    $(BTSTACK_ROOT)/transport/hci_transport_h4.c \
    $(BTSTACK_ROOT)/transport/btstack_slip.c \
    $(BTSTACK_ROOT)/transport/btstack_uart_slip_wrapper.c

BTSTACK_GAP_SOURCES := \
    $(BTSTACK_ROOT)/gap/btstack_gap.c \
    $(BTSTACK_ROOT)/gap/btstack_gap_ad_data.c \
    $(BTSTACK_ROOT)/gap/btstack_gap_classic.c \
    $(BTSTACK_ROOT)/gap/btstack_gap_le.c

BTSTACK_UPSTREAM_CORE_SOURCES := \
    $(BTSTACK_ROOT)/src/ad_parser.c \
    $(BTSTACK_ROOT)/src/btstack_linked_list.c \
    $(BTSTACK_ROOT)/src/btstack_memory.c \
    $(BTSTACK_ROOT)/src/btstack_memory_pool.c \
    $(BTSTACK_ROOT)/src/btstack_run_loop.c \
    $(BTSTACK_ROOT)/src/btstack_util.c \
    $(BTSTACK_ROOT)/src/hci.c \
    $(BTSTACK_ROOT)/src/hci_cmd.c \
    $(BTSTACK_ROOT)/src/hci_dump.c \
    $(BTSTACK_ROOT)/src/hci_event.c \
    $(BTSTACK_ROOT)/src/hci_event_builder.c \
    $(BTSTACK_ROOT)/src/l2cap.c \
    $(BTSTACK_ROOT)/src/l2cap_signaling.c

BTSTACK_BLE_CORE_SOURCES := \
    $(BTSTACK_ROOT)/src/ble/att_db.c \
    $(BTSTACK_ROOT)/src/ble/att_db_util.c \
    $(BTSTACK_ROOT)/src/ble/att_dispatch.c \
    $(BTSTACK_ROOT)/src/ble/att_server.c \
    $(BTSTACK_ROOT)/src/ble/gatt_client.c \
    $(BTSTACK_ROOT)/src/ble/sm.c \
    $(BTSTACK_ROOT)/src/ble/le_device_db_memory.c \
    $(BTSTACK_ROOT)/src/btstack_crypto.c \
    $(BTSTACK_ROOT)/src/btstack_tlv.c

BTSTACK_CLASSIC_SOURCES := \
    $(BTSTACK_ROOT)/src/classic/sdp_util.c \
    $(BTSTACK_ROOT)/src/classic/sdp_server.c \
    $(BTSTACK_ROOT)/src/classic/sdp_client.c \
    $(BTSTACK_ROOT)/src/classic/sdp_client_rfcomm.c \
    $(BTSTACK_ROOT)/src/classic/rfcomm.c \
    $(BTSTACK_ROOT)/src/classic/avdtp.c \
    $(BTSTACK_ROOT)/src/classic/avdtp_util.c \
    $(BTSTACK_ROOT)/src/classic/avdtp_acceptor.c \
    $(BTSTACK_ROOT)/src/classic/avdtp_initiator.c \
    $(BTSTACK_ROOT)/src/classic/avdtp_sink.c \
    $(BTSTACK_ROOT)/src/classic/a2dp.c \
    $(BTSTACK_ROOT)/src/classic/a2dp_sink.c \
    $(BTSTACK_ROOT)/src/classic/avrcp.c \
    $(BTSTACK_ROOT)/src/classic/avrcp_controller.c \
    $(BTSTACK_ROOT)/src/classic/avrcp_target.c \
    $(BTSTACK_ROOT)/src/classic/hfp.c \
    $(BTSTACK_ROOT)/src/classic/hfp_hf.c

BTSTACK_COVER_ART_NATIVE_SOURCES := \
    $(BTSTACK_ROOT)/src/classic/avrcp_cover_art_client.c

BTSTACK_BIP_SOURCES := \
    $(BTSTACK_ROOT)/profiles/btstack_bip_client.c \
    $(BTSTACK_ROOT)/profiles/btstack_avrcp_cover_art.c

# Selected by the API and public AT builds. The Legacy AT archive intentionally
# omits Cover Art until that compatibility build is updated separately.
BTSTACK_COVER_ART_SOURCES := \
    $(BTSTACK_COVER_ART_NATIVE_SOURCES) \
    $(BTSTACK_BIP_SOURCES)

BTSTACK_XML_SOURCES := \
    $(BTSTACK_ROOT)/3rd-party/yxml/yxml.c

BTSTACK_PROFILE_SOURCES := \
    $(BTSTACK_ROOT)/profiles/btstack_a2dp_media.c \
    $(BTSTACK_ROOT)/profiles/btstack_a2dp_sink.c \
    $(BTSTACK_ROOT)/profiles/btstack_avrcp.c \
    $(BTSTACK_ROOT)/profiles/btstack_avrcp_controller.c \
    $(BTSTACK_ROOT)/profiles/btstack_avrcp_target.c \
    $(BTSTACK_ROOT)/core/btstack_profile_manager.c \
    $(BTSTACK_ROOT)/profiles/btstack_gatt_client.c \
    $(BTSTACK_ROOT)/profiles/btstack_gatt_server.c \
    $(BTSTACK_ROOT)/profiles/btstack_hfp_hf.c \
    $(BTSTACK_ROOT)/profiles/btstack_hid_device.c \
    $(BTSTACK_ROOT)/profiles/btstack_hogp_device.c \
    $(BTSTACK_ROOT)/profiles/btstack_pbap_client.c \
    $(BTSTACK_ROOT)/profiles/btstack_vcard_parser.c

BTSTACK_PBAP_DEPENDENCY_SOURCES := \
    $(BTSTACK_ROOT)/src/classic/pbap_client.c \
    $(BTSTACK_ROOT)/src/classic/goep_client.c \
    $(BTSTACK_ROOT)/src/classic/obex_message_builder.c \
    $(BTSTACK_ROOT)/src/classic/obex_parser.c \
    $(BTSTACK_ROOT)/src/classic/obex_iterator.c \
    $(BTSTACK_ROOT)/src/classic/obex_srm_client.c \
    $(BTSTACK_ROOT)/3rd-party/md5/md5.c

BTSTACK_SBC_SOURCES := \
    $(BTSTACK_ROOT)/src/classic/btstack_sbc_decoder_bluedroid.c \
    $(BTSTACK_ROOT)/src/classic/btstack_sbc_plc.c \
    $(BTSTACK_ROOT)/3rd-party/bluedroid/decoder/srce/alloc.c \
    $(BTSTACK_ROOT)/3rd-party/bluedroid/decoder/srce/bitalloc.c \
    $(BTSTACK_ROOT)/3rd-party/bluedroid/decoder/srce/bitalloc-sbc.c \
    $(BTSTACK_ROOT)/3rd-party/bluedroid/decoder/srce/bitstream-decode.c \
    $(BTSTACK_ROOT)/3rd-party/bluedroid/decoder/srce/decoder-oina.c \
    $(BTSTACK_ROOT)/3rd-party/bluedroid/decoder/srce/decoder-private.c \
    $(BTSTACK_ROOT)/3rd-party/bluedroid/decoder/srce/decoder-sbc.c \
    $(BTSTACK_ROOT)/3rd-party/bluedroid/decoder/srce/dequant.c \
    $(BTSTACK_ROOT)/3rd-party/bluedroid/decoder/srce/framing.c \
    $(BTSTACK_ROOT)/3rd-party/bluedroid/decoder/srce/framing-sbc.c \
    $(BTSTACK_ROOT)/3rd-party/bluedroid/decoder/srce/oi_codec_version.c \
    $(BTSTACK_ROOT)/3rd-party/bluedroid/decoder/srce/synthesis-sbc.c \
    $(BTSTACK_ROOT)/3rd-party/bluedroid/decoder/srce/synthesis-dct8.c \
    $(BTSTACK_ROOT)/3rd-party/bluedroid/decoder/srce/synthesis-8-generated.c

BTSTACK_COMMON_SOURCES := \
    $(BTSTACK_ADAPTER_SOURCES) \
    $(BTSTACK_EVENT_SOURCES) \
    $(BTSTACK_RUNTIME_SOURCES) \
    $(BTSTACK_TRANSPORT_SOURCES) \
    $(BTSTACK_GAP_SOURCES) \
    $(BTSTACK_UPSTREAM_CORE_SOURCES) \
    $(BTSTACK_BLE_CORE_SOURCES) \
    $(BTSTACK_CLASSIC_SOURCES) \
    $(BTSTACK_PROFILE_SOURCES) \
    $(BTSTACK_PBAP_DEPENDENCY_SOURCES) \
    $(BTSTACK_XML_SOURCES) \
    $(BTSTACK_SBC_SOURCES)

# Deprecated compatibility alias. This is common code only; select exactly
# one frontend and one target port separately.
BTSTACK_SOURCES := $(BTSTACK_COMMON_SOURCES)
