# Migration to tcn_schema 0.10.2

**Additive. Nothing to change unless you want the voice-client protocol.** 0.10.2 adds the package
`conversation_msgs`, the realtime conversational pipeline's protocol. No existing type, field, enum value or key
changes; the SQE encoding annotation stays `;tcn_schema=0.10`, so 0.10.0, 0.10.1 and 0.10.2 peers talk to each
other unchanged. 0.10.2 builds on 0.10.1 and includes its changes (the SQE transaction log, engine B92:
[MIGRATION-0.10.1.md](MIGRATION-0.10.1.md)).

## What is new

`src/dds/custom/conversation_msgs/`, Conan component `tcn_schema::conversation_msgs` (library
`tcn_schema_conversation_msgs`, no dependency on another package):

| folder | module | files |
|---|---|---|
| `msg/` | `conversation_msgs::msg` | `common`, `control`, `audio`, `stt`, `llm`, `tts`, `metrics` |
| `rpc/` | `conversation_msgs::rpc` | `discovery` (`AppServerInfo`) |

The files are the conversational pipeline's `idl/*.idl` (module `nv::voice::v1`) with the module path, the include
guards and the `#include` paths changed. Type names, field names, field order and enum order are the pipeline's, so
the wire is the pipeline's (plain CDR carries no type, member or enumerator names), with three textual deviations:

- `RetrievalPolicy`'s first enumerator is `auto_` (the pipeline's `auto`): `auto` is a C++ keyword and fastddsgen
  emits enumerators verbatim. Same value (0); a JSON mapping that prints enumerator names prints `auto_`.
- `HealthStatus.component` is written `_component`: `component` is an IDL keyword, and the leading underscore is
  IDL's escape. The member is `component` (fastddsgen's C++ accessor is `component()`).
- `AppServerInfo` (now in `rpc`) names its `MessageHeader` and `AudioFormat` members as
  `conversation_msgs::msg::MessageHeader` / `conversation_msgs::msg::AudioFormat` (IDL scoping).

## The plain-CDR wire profile

`conversation_msgs` messages travel as the pipeline sends them: **OMG CDR v1 little-endian without the 4-byte
encapsulation header** (the payload starts with the first member; alignment is relative to the payload's start),
zenoh encoding `application/cdr` with no type segment and no `tcn_schema` parameter (the key determines the type).
Every other tcn_schema type keeps the encapsulation (`00 01 00 00`) and its typed encoding
`application/cdr;<type>;tcn_schema=0.10`.

With the fastddsgen classes: serialize without `serialize_encapsulation()` / `read_encapsulation()`, e.g.

```cpp
eprosima::fastcdr::Cdr cdr(buffer, eprosima::fastcdr::Cdr::LITTLE_ENDIANNESS, eprosima::fastcdr::DDS_CDR);
cdr << message;  // no cdr.serialize_encapsulation()
```

## To adopt

Nothing for existing consumers. To use the voice protocol, link `tcn_schema::conversation_msgs` and include
`conversation_msgs/msg/<file>.h` / `conversation_msgs/rpc/discovery.h`.
