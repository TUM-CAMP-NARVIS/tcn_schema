# The realtime conversational pipeline's voice-client protocol (its IDL, module nv::voice::v1, moved here unchanged
# but for the module path): OMG CDR v1 little-endian *without* the 4-byte encapsulation header on the wire, zenoh
# encoding application/cdr (see MIGRATION-0.10.2.md). No dependency on another tcn_schema package.

tcn_add_idl_package(
        NAME conversation_msgs
        MSG
            "msg/common.idl"
            "msg/control.idl"
            "msg/audio.idl"
            "msg/stt.idl"
            "msg/llm.idl"
            "msg/tts.idl"
            "msg/metrics.idl"
        RPC
            "rpc/discovery.idl"
        )
