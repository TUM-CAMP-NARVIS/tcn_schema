# Migration to tcn_schema 0.9.0

The parameter protocol, finished: a descriptor a generic UI can build an editor
from, unsigned values, per-key set results, change notifications, and three
process-level verbs — `graph`, `info` (with a liveliness token) and `streams`.

**All additive.** New enums and structs, and two cases appended to
`ParameterValueType` / `ParameterValue`. No existing struct gained, lost or
moved a member, and every 0.8.0 ordinal is unchanged. The CDR bytes of every
0.8.0 message are identical under 0.9.0 (checked: `GenericParameterReply` with
one value of each 0.8.0 case, and `ServiceInfoReply`, encoded with both
packages).

**Nothing has to be rebuilt to keep working.** The 0.8.0 verbs reply with the
same types as before. A component is affected only when it starts using a 0.9.0
verb, and the one rule to keep while doing so is in A2: never put a
`PVT_UINT32` / `PVT_UINT64` value into a reply a 0.8.0 peer may read.

Origin: holoscan-tcn-applications `docs/migration/28-tcn-schema-proposal.md`
(A1–A5, corrections B1–B8) and its design 29, the parameter-control UI (A6, A7).

---

## Why

The parameter interface (`describe`, `parameters/get`, `parameters/set`) was
enough for a hand-built editor that already knew each service. It is not enough
for a generic one, and a generic one now exists (holoscan's parameter-control
UI, the slang viewer's property editor):

- **`describe` cannot say what a valid value is.** No range, no step, no unit,
  no description, no "read-only" or "applies after restart". Enum labels are
  there, their integer values are not. Every k4a describe carries the same
  `@todo` for ranges.
- **Unsigned values are narrowed.** A `uint32` goes out as `PVT_INT32`, a
  `uint64` as `PVT_INT64`, with no range check: values above `INT32_MAX` /
  `INT64_MAX` change sign.
- **`set` cannot say what went wrong.** It replies `NullReply`; one bad key
  fails the whole request with no reason, and a client cannot tell "rejected"
  from "out of range" from "unknown key".
- **A client must poll** to see a change made by another client.
- **Nothing describes a process.** Presence lists services and a counter: no
  app, host, pid or version, and no leave signal — a crashed process is only
  noticed when its presence stops. Nothing says what a process publishes, so a
  client cannot offer a preview of its streams.

---

## Names used below

- `<P>` — a process's service root: `<prefix>/<app>` by convention. ArteKMed:
  the `topic_prefix` its `AppController` is constructed with (e.g.
  `<prefix>/k4a_capture_multi`). Holoscan: `tcn/loc/holoscan/<app>`, adjusted by
  `--node` / `--param-prefix`.
- `<P>/rpc` — the RPC root. ArteKMed's `ServiceController` already appends
  `/rpc` to its prefix and serves `<P>/rpc/**`. Service verbs are
  `<P>/rpc/service/<name|*>/...`, process verbs `<P>/rpc/<verb>`.

## Summary

| Item | Types | IDL | Verb / key |
|---|---|---|---|
| A1 descriptor | `ParameterAccess`, `EnumEntry`, `ParameterDescriptor`, `ParameterDescriptorList`, `ParameterDescriptorReply` | `tcnart_msgs/rpc/Requests.idl` | `<P>/rpc/service/<name\|*>/parameters/describe` |
| A2 unsigned | `PVT_UINT32` (7), `PVT_UINT64` (8); cases `uint32_value`, `uint64_value` | `tcnart_msgs/rpc/Requests.idl` | — |
| A3 set results | `ParameterSetStatus`, `ParameterSetResult`, `ParameterSetReply` | `tcnart_msgs/rpc/Requests.idl` | `<P>/rpc/service/<name\|*>/parameters/set_ex` |
| A4 change notification | `ParameterChanged` | `tcnart_msgs/rpc/Requests.idl` | publication on `<P>/str/service/<name>/parameters` |
| A5 graph | `GraphReply` | `pcpd_msgs/rpc/ServiceController.idl` | `<P>/rpc/graph` |
| A6 process info | `ProcessInfo`, `ProcessInfoReply` | `pcpd_msgs/rpc/ServiceController.idl` | liveliness token `<P>/rpc`; verb `<P>/rpc/info` |
| A7 streams | `StreamAnnouncement`, `StreamAnnouncementReply` | `pcpd_msgs/rpc/ServiceController.idl` | `<P>/rpc/streams` |

`TcnartPresenceMessage` (`<P>/str/presence`) is unchanged and stays.

---

## A1 — a parameter described fully enough for a generic UI

```idl
enum ParameterAccess { PA_READ_WRITE_LIVE, PA_READ_WRITE_RESTART, PA_READ_ONLY };

struct EnumEntry { string label; int64 value; };

struct ParameterDescriptor {
  string key;
  ParameterValueType value_type;
  ParameterAccess access;
  string description;                             // one line, may be empty
  string unit;                                    // "m", "ms", "px", "" ...
  boolean has_minimum;  ParameterValue minimum;   // inclusive, same type as value_type
  boolean has_maximum;  ParameterValue maximum;   // inclusive
  boolean has_step;     ParameterValue step;      // UI hint; absent = continuous
  sequence<EnumEntry> enum_entries;               // enums: value_type PVT_STRING
  sequence<ParameterValue> valid_entries;         // other discrete sets (as ParameterSchema)
  ParameterValue default_value;
  uint64 generation;                              // change counter of the current value
};

struct ParameterDescriptorList  { sequence<ParameterDescriptor> parameters; };
struct ParameterDescriptorReply { ParameterDescriptorList value; RPCResponseStatus status; };
```

**Verb** `parameters/describe`: request `StringListRequest` (keys; empty = all),
reply `ParameterDescriptorReply`.

**Who must rebuild:** only a component that serves or calls the new verb. The
0.8.0 `describe` (`ServiceInfoReply` with a `ParameterListSchema`) is
unchanged and stays.

**Rules:**
- An enum is `value_type = PVT_STRING`; its values are sent and received as the
  label (`string_value`), `enum_entries` lists every label with its integer.
- `minimum`, `maximum`, `step` and `default_value` carry the same union case as
  `value_type`.
- `generation` increases whenever the value changes, from any client; a UI uses
  it to notice an edit it did not make.

### Deviation from the proposal: no `@optional`

The proposal wrote `@optional ParameterValue minimum;` (and `maximum`, `step`).
fastddsgen 3.0.0 accepts `@optional` and generates
`eprosima::fastcdr::optional<>`, but **it does not round-trip under the
encoding every peer uses** (fast-cdr 2.0.0, `CdrVersion::DDS_CDR`, i.e. XCDR1
plain CDR, as `pcpd::serialization::CdrBufferWriter` and holoscan's
`tcn::zenoh::encode` write it): an absent member is written as nothing and the
reader reads it anyway; a present member is written without a presence marker.
Checked with `{ string key; @optional V minimum; uint64 g; }`: decoding the
absent case throws `NotEnoughMemoryException`; the present case decodes as
absent with a garbage `g`. It would also put XTypes optionals in front of the
hand-written pycdr2 mirrors, which have none.

So each optional bound is a plain member preceded by a `boolean has_*` flag.
When the flag is `false` the member is default-constructed (discriminator
`PVT_STRING`, empty string) and **must be ignored**; a writer leaves it at the
default. A reader must never infer presence from the value.

## A2 — unsigned integers without narrowing

```idl
enum ParameterValueType {
  PVT_STRING, PVT_INT32, PVT_INT64, PVT_FLOAT, PVT_DOUBLE, PVT_BOOL, PVT_STRUCT,
  PVT_UINT32,   // 7, appended
  PVT_UINT64    // 8, appended
};

union ParameterValue switch(ParameterValueType) {
  ...                                   // 0.8.0 cases unchanged
  case PVT_UINT32: uint32 uint32_value;
  case PVT_UINT64: uint64 uint64_value;
};
```

**Appended, never inserted** — the discriminator is the union's first four
bytes, so renumbering would make old peers misread.

**The hazard: an old reader does not fail, it misreads.** MIGRATION-0.5.0 says
"an unrecognised discriminant fails the decode". For the code fastddsgen 3.0.0
generates, it does not: the union reader matches no case, reads no value, and
continues — every member after it in the message is then read from the wrong
offset, silently. (Checked: a struct `{ V v; uint32 g; }` with an unknown
discriminator followed by a 4-byte value decodes with `g` = that value.)

Hence the rule, for every server:

- send `PVT_UINT32` / `PVT_UINT64` **only** in replies of the 0.9.0 verbs
  (`parameters/describe`, `parameters/set_ex`) and in `ParameterChanged` —
  only a 0.9.0 client asks for or subscribes to those;
- in replies of the 0.8.0 verbs (`describe`, `parameters/get`, component
  verbs), keep sending unsigned values as `PVT_INT32` / `PVT_INT64`, and
  range-check them: a value that does not fit is an error, not a sign flip;
- **accept** `PVT_INT32` / `PVT_INT64` for an unsigned field from older clients,
  range-checking the value (negative or too large: reject).

`parameters/get` gives a 0.9.0 client no way to say it understands unsigned
cases; a client that wants exact unsigned values reads them from
`parameters/describe` (`default_value`, bounds) and `ParameterChanged`, or
converts the `INT64` it gets back using `value_type` from the descriptor.

## A3 — per-key set results

```idl
enum ParameterSetStatus {
  PSS_APPLIED,        // in effect
  PSS_UNCHANGED,      // equal to the current value
  PSS_PENDING,        // accepted, not yet in effect within the server's wait
  PSS_OUT_OF_RANGE,
  PSS_TYPE_MISMATCH,
  PSS_UNKNOWN_KEY,
  PSS_READ_ONLY,
  PSS_REJECTED        // other; see message
};
struct ParameterSetResult { string key; ParameterSetStatus status; string message; };
struct ParameterSetReply  { sequence<ParameterSetResult> results; RPCResponseStatus status; };
```

**Verb** `parameters/set_ex`: request `GenericParameterRequest` (same as
`parameters/set`), reply `ParameterSetReply` with one result per requested key,
in request order. `status` is `RPC_STATUS_SUCCESS` only if every key is
`PSS_APPLIED` or `PSS_UNCHANGED`. `message` is a human-readable reason, empty on
success.

**Who must rebuild:** a server that adds the verb, a client that calls it.
`parameters/set` (`NullReply`) stays as it is.

## A4 — change notification

```idl
struct ParameterChanged { string service; ParameterList value; uint64 generation; };
```

Published (CDR) on `<P>/str/service/<name>/parameters` whenever one or more
values take effect — from any client, or from the process itself. `value`
holds only the keys that changed, with their new values; `generation` is the
same counter as `ParameterDescriptor.generation`. A UI subscribes instead of
polling and re-reads with `parameters/get` if it sees a generation gap.

## A5 — dataflow graph (optional)

```idl
struct GraphReply { string graph_json; string schema; RPCResponseStatus status; };
```

**Verb** `<P>/rpc/graph`. `graph_json` is a versioned JSON document named by
`schema` (holoscan: `"tcn.plan_graph.v1"`, specified in holoscan-tcn-applications
`docs/users/configuration/plan-graph.md`: nodes, ports, edges with queue depths,
partitions, tunables, streams). The graph stays JSON inside CDR because it is
nested and app-specific. Optional: a client shows an app without the verb from
its services list alone.

## A6 — process info and liveliness

```idl
struct ProcessInfo {
  string app;
  string node;           // instance name; empty when there is one
  string host;
  uint32 pid;
  string started_at;     // ISO 8601, UTC
  string version;        // e.g. git describe
  string plan_hash;      // empty when the app has no plan
  uint64 generation;     // parameter change counter
  string rpc_root;       // "<P>/rpc"
};
struct ProcessInfoReply { ProcessInfo value; RPCResponseStatus status; };
```

The convention:

- **Liveliness token** `<P>/rpc`, declared when the RPC queryable is declared
  and kept until it is undeclared. Zenoh drops the token about a second after a
  clean exit, and when the session lease expires after a crash or a network
  loss (10 s by default). That is the leave signal presence lacks. A client
  subscribes to liveliness on `**/rpc` **with history**, so processes that are
  already up appear at once.
- **Verb** `<P>/rpc/info` replies `ProcessInfoReply`.
- **Identity:** same root, different `pid` or `started_at` → a restart; two
  different replies on one root at the same time → a duplicate root (two
  instances without distinct node names).
- **Presence stays.** A process without a token (any 0.8.0 build) is still
  found through `<P>/str/presence`, as *presence only*.

This settles proposal item B8 (one RPC-root convention) without changing the
presence message: the root is the token key and `rpc_root`.

## A7 — stream announcements

```idl
struct StreamAnnouncement {
  string op;         // operator / component that publishes it
  string topic;      // full key expression
  string payload;    // payload type name
  string role;       // "data" | "descriptor" | "state"
  string encoding;   // e.g. "cdr", "json", a codec name
  string mode;       // "stream" | "latched"
};
struct StreamAnnouncementReply { sequence<StreamAnnouncement> streams; RPCResponseStatus status; };
```

**Verb** `<P>/rpc/streams`: the streams a process publishes, each announced by
its sender when it starts. `role` and `mode` use the same values as the
`streams` entries of `tcn.plan_graph.v1`. A client uses it to offer a preview
without knowing the pipeline's configuration.

---

## Canonical JSON mapping

Every verb may also be answered in JSON: a request with encoding `text/json`
gets a JSON reply (both ArteKMed's `ServiceController` and the holoscan
parameter service already do this). 0.8.0 has a hand-written JSON layout
(`pcpd_zenoh/json_cdr_serialization.h`). For the 0.9.0 types the mapping is
defined here; it agrees with the 0.8.0 layout for every type they share:

- **A struct** is a JSON object whose keys are the IDL member names, written in
  IDL order. (Order is for readability; readers must not depend on it.)
- **A nested struct** is a nested object; a `sequence<T>` is an array.
- **An enum** is its ordinal as a JSON integer (`RPCResponseStatus`,
  `ParameterAccess`, `ParameterSetStatus`, `ParameterValueType`).
- **A union** (`ParameterValue`) is `{"type": <discriminator ordinal>,
  "value": <member>}`; the member name (`int32_value`, ...) does not appear.
  `PVT_STRUCT` carries a `ParameterList` object as `value`.
- **`has_*` flags** are booleans like any member. A writer may write `null` for
  a member whose flag is `false`; a reader ignores the member whenever its flag
  is `false`.
- **Integers** are JSON numbers. `uint64` / `int64` above 2^53 lose precision
  in JavaScript readers (the web frontend); C++ (rapidjson, nlohmann) and Python
  keep them exact.
- **Extra keys** are allowed; readers ignore keys they do not know. The
  holoscan `info` verb, for example, adds `"schema": "tcn.params.info.v1"`.

Example, a `ProcessInfoReply`:

```json
{"value": {"app": "stereo_depth", "node": "", "host": "capture01", "pid": 4242,
           "started_at": "2026-11-01T09:00:00Z", "version": "v1.4.0-12-gabc123",
           "plan_hash": "9f2c...", "generation": 3,
           "rpc_root": "tcn/loc/holoscan/stereo_depth/rpc"},
 "status": 0}
```

### Known exceptions

- **`ServiceInfoReply` / `ComponentInfoReply` (0.8.0, unchanged):** the JSON
  form flattens `schema.parameters` to a top-level `"parameters"` array. It
  stays for compatibility.
- **`GraphReply` in holoscan (predates 0.9.0):** holoscan's `graph` verb
  replies JSON as `{"graph": <the document as an object>, "status": <code>}`,
  not `{"graph_json": "<the document as a string>", ...}`. A reader of the JSON
  form accepts both: an object under `"graph"`, or a string under
  `"graph_json"` to parse. *(Whether holoscan switches to the canonical form is
  open; see holoscan design 29.)*

---

## To adopt — servers

### ArteKMed: `libs/pcpd_zenoh` (`ServiceController`, JSON layout)

The shared base of every ArteKMed RPC server. Paths relative to the `artekmed`
repository.

1. **Liveliness token (A6).** `ServiceController::start()`
   (`libs/pcpd_zenoh/src/service_controller.cpp`) declares the queryable on
   `m_topic_prefix + "/**"`, where `m_topic_prefix` is already `<P>/rpc`.
   Declare a liveliness token on `m_topic_prefix` next to it, keep it as a
   member beside `m_queryable`, and drop it in `stop()`.
2. **JSON layout (canonical mapping).** Add `typeMap<>` specialisations to
   `libs/pcpd_zenoh/include/pcpd_zenoh/json_cdr_serialization.h` for every new
   type a server sends: `ParameterDescriptor(Reply|List)`, `EnumEntry`,
   `ParameterSetResult`, `ParameterSetReply`, `ParameterChanged`,
   `ProcessInfo(Reply)`, `StreamAnnouncement(Reply)`, `GraphReply`; and the
   `PVT_UINT32` / `PVT_UINT64` cases in `typeMap<ParameterValue>` (serialize and
   deserialize switches, `json_cdr_serialization.h:1191`ff).
3. **Fix while adopting (B2):** `ServiceController::query_handler` and
   `process_request` (`service_controller.cpp:101`, `:186`) call
   `query.get_payload()->get()` and `query.get_encoding().value()` without
   checking either is present. A query with no payload (the slang viewer's
   reads, a bare `z_get`) then fails. Treat a missing payload as `NullRequest`
   / an empty `StringListRequest`, and a missing encoding as CDR.
4. **Fix while adopting (B4):** the JSON reader of `GenericParameterReply`
   checks `reader.HasMember("key")` / `("value")` on the outer object instead of
   the array element `v` (`json_cdr_serialization.h:1432`). Check `v`.
5. **Fix while adopting (B7):** the hand-written layout is now the documented
   canonical mapping (above); keep new specialisations to it.

### ArteKMed apps (`AppController` per app, `pcpd::facade` `AppController`)

The parameter handlers are copies, one per app:

| App / library | File | Parameter handlers |
|---|---|---|
| k4a capture multi | `apps/pcp_shm_k4a_capture_multi/include/app/app_controller.h` | active |
| orbbec capture multi | `apps/pcp_shm_orbbec_capture_multi/include/app/app_controller.h` | active |
| pcp daemon (facade) | `libs/pcpd/include/pcpd/facade/facade_controller.h` | active (service and component verbs) |
| zenoh sender | `apps/pcp_shm_zenoh_sender/include/app/app_controller.h` | stubbed (empty schema; RTTR code commented out) |
| zenoh receiver | `apps/pcp_shm_zenoh_receiver/include/app/app_controller.h` | stubbed |
| mkv recorder | `apps/pcp_shm_mkv_record_multi/include/app/app_controller.h` | stubbed |
| pointcloud meshing | `apps/pcp_shm_pointcloud_meshing/include/app/app_controller.h` | own handlers (not reviewed in detail) |

Line numbers below are from the k4a file; the other copies are the same code at
nearby lines.

1. **`parameters/describe` (A1).** Add a `ServiceDescribeExCommand :
   ServiceContextCommand<StringListRequest, ParameterDescriptorReply>` next to
   `ServiceInfoCommand` (k4a `:292`), built from the same RTTR walk:
   - `value_type` from `getParameterTypeFromRTTR` (k4a `:232`), with the
     unsigned mapping of item 3;
   - `enum_entries` from `t.get_enumeration()` — label `to_string()`, value
     `to_int64` / the underlying integer;
   - ranges, step, unit, description, access: from RTTR metadata on the
     property. This is the `@todo` at k4a `:326` (**B6**). *Not verified* which
     RTTR metadata keys the properties carry today; probably none, so this needs
     `rttr::metadata(...)` added at each registration;
   - `access`: `PA_READ_ONLY` for read-only RTTR properties
     (`prop.is_readonly()`), else `PA_READ_WRITE_LIVE` unless the service only
     applies it on restart.
   Register it in the `AppController` constructor (k4a `:778`ff) for
   `service/*/parameters/describe` and `service/<word>/parameters/describe`
   (facade: also `component/...`).
2. **`parameters/set_ex` (A3).** Add `ServiceSetParametersExCommand :
   ServiceContextCommand<GenericParameterRequest, ParameterSetReply>` beside
   `ServiceSetParametersCommand` (k4a `:447`). Same loop, but record a
   `ParameterSetResult` per key instead of `success = false; break;`: unknown
   key → `PSS_UNKNOWN_KEY`, wrong case → `PSS_TYPE_MISMATCH`, bad enum label →
   `PSS_OUT_OF_RANGE`, read-only → `PSS_READ_ONLY`, equal value →
   `PSS_UNCHANGED`. Register `service/*/parameters/set_ex` and
   `service/<word>/parameters/set_ex`.
3. **Unsigned types (A2, B5).** `getParameterTypeFromRTTR` (k4a `:232`,
   facade `:163`) maps `uint32_t` → `PVT_INT32` and `uint64_t` → `PVT_INT64`
   without a range check. For the 0.9.0 verbs map them to `PVT_UINT32` /
   `PVT_UINT64`; for the 0.8.0 verbs (`describe`, `parameters/get`) keep the
   signed case and reject values that do not fit. In the set handlers accept
   both the unsigned case and the signed one with a range check.
   `getRTTRTypeFromParameter` (k4a `:271`) gains the two unsigned cases.
4. **Fix while adopting (B3):** the enum branch of the set handler checks
   `prop_value._d() == PVT_DOUBLE` and then reads `string_value()` (k4a `:579`;
   facade `:370`, also `:363`; commented copies in sender/receiver/recorder).
   An enum is `PVT_STRING`. As written, a correct client can never set an enum.
5. **`ParameterChanged` (A4).** After a successful set (both `set` and
   `set_ex`), publish `ParameterChanged` on
   `<prefix>/str/service/<name>/parameters`. The publisher can live beside
   `m_presence_topic` in `AppController` (k4a `:761`). Keep a per-service
   generation counter; increment on every applied change.
6. **`info` (A6).** Add a `ProcessInfoCommand : AppControllerCommand<NullRequest,
   ProcessInfoReply>` and register `match("info")`. `app` and `rpc_root` from
   the controller's entity name / topic prefix; `pid`, `host`, `started_at` from
   the process; `version` from the build (git describe if the build exposes
   one — *not verified*); `plan_hash` empty.
7. **`streams` (A7).** Optional for ArteKMed. If a process publishes streams a
   client should preview (k4a and orbbec capture, the zenoh sender), register
   `match("streams")` with a `StreamAnnouncementReply` built from the topics the
   app already composes for its publishers.
8. **`graph` (A5).** Optional; skip unless an app has a pipeline to describe.
9. **Fix while adopting (B8):** RPC roots differ between apps today (e.g.
   `<prefix>/k4a_capture/k4a_capture/rpc` vs `.../pcp_daemon/rpc`, as observed
   in design 28). With the token and `info` the root no longer needs to be
   guessed; still, construct every `AppController` with `<prefix>/<app>` so
   roots read the same.

### Holoscan: `libs/params_service` (holoscan-tcn-applications)

1. **Routing.** `Verb` (`libs/params_service/src/dispatch.hpp:15`) and
   `route()` (`:68`ff) gain `parameters/describe`, `parameters/set_ex`, `info`
   and `streams`.
2. **`parameters/describe` (A1).** Build `ParameterDescriptorReply` from the
   same field table as `describe_ext` (`Impl::describe_ext`,
   `parameter_service.cpp:543`; JSON in `detail::describe_ext_json`,
   `json_codec.cpp`): `live` → `access`, `min`/`max` → `has_minimum`/`minimum`
   etc., `enum` labels → `enum_entries`, `description`, `unit`, `generation`.
   Keep `describe_ext` for one release.
3. **`parameters/set_ex` (A3).** `Impl::set` (`parameter_service.cpp:479`)
   already has a per-key outcome (`holoscan::ParamUpdateStatus`,
   `status_text`, `:187`); map it to `ParameterSetStatus` and return a
   `ParameterSetReply`.
4. **Unsigned (A2).** `wire_type` (`type_map.cpp:111`) maps unsigned widths to
   `PVT_INT32`/`PVT_INT64` (`:120`); `to_wire` (`:130`) and `from_wire` (`:171`).
   Use `PVT_UINT32`/`PVT_UINT64` on the 0.9.0 verbs only; keep the signed,
   range-checked mapping on `describe` / `parameters/get`; accept both on set.
   The JSON codec (`json_codec.cpp`, `value_json` / `value_from_json`) gains
   the two cases.
5. **`ParameterChanged` (A4).** Publish after each applied change, on
   `<P>/str/service/<name>/parameters`.
6. **Token and `info` (A6).** Declare the `<P>/rpc` token in
   `ParameterService::start` next to the queryable; serve `info` in
   `Impl::on_query` (`parameter_service.cpp:320`), JSON and CDR.
7. **`streams` (A7).** Serve the process-wide announcement registry
   (`tcn::streams::announce`, design 29 D29.8).
8. **`graph` (A5).** Already served (`graph_reply`, `parameter_service.cpp:46`),
   JSON only. Add the CDR form as `GraphReply`; see the JSON exception above.

---

## To adopt — clients

The rule for every client: **prefer the 0.9.0 form, fall back to the 0.8.0
one**, because ArteKMed and holoscan processes of both versions will be on the
network together.

| Need | 0.9.0 | Fallback (0.8.0 server) |
|---|---|---|
| describe | `parameters/describe` | `describe` (+ holoscan `parameters/describe_ext`) |
| set | `parameters/set_ex` | `parameters/set` (one status for all keys) |
| see changes | subscribe `<P>/str/service/<name>/parameters` | poll `parameters/get` |
| find processes | liveliness `**/rpc` with history + `<P>/rpc/info` | `**/str/presence` (+ `ping`) |
| what it publishes | `<P>/rpc/streams` | — |
| pipeline | `<P>/rpc/graph` | services list only |

A fallback is taken when the 0.9.0 verb gets **no reply** or an error reply —
0.8.0 servers reply to an unknown verb with a 500 / `NullReply` error, or not at
all (pattern queries).

### Slang viewer: `apps/pcp_slang_viewer/include/zenoh/ZenohPropertyClient.h` (artekmed)

1. **Fix while adopting (B1):** `enum class ParamType` (`:37`) has Bool = 0 …
   String = 5, which swaps bool and string against the IDL (STRING = 0 …
   BOOL = 5). Use the IDL ordinals, and add `UInt32 = 7`, `UInt64 = 8`.
2. **describe → `describeService` / `describeComponent`** (`:122`, `:128`): try
   `.../parameters/describe` first, fall back to `.../describe`; fill
   `ParamDescriptor` (`:59`) with range, step, unit, description, access, enum
   entries.
3. **set:** the `parameters/set` calls (`:150`, `:159`) → `parameters/set_ex`,
   showing per-key messages; fall back to `set`.
4. **Changes:** `PropertyEditorWidget::refresh()`
   (`include/gui/PropertyEditorWidget.h:53`) is manual today; subscribe to
   `ParameterChanged` instead.
5. Its reads send no payload (`queryGet`, `:167`) — that is what B2 breaks on
   the server side; fixing B2 on the server is the right fix, nothing to change
   here.

### ArteKMed web frontend: `artekmed_web_frontend/packages/zenoh-client`

JSON client (`handlers/get-entity-descriptions.ts`, `set-entity-parameters.ts`):
describe → `parameters/describe` with fallback; `set_ex` with fallback; treat
`uint64` values above 2^53 as inexact (JSON numbers in JavaScript). *Not
reviewed beyond those two files.*

### ArteKMed Python clients: `artekmed_python_client`, `artekmed_zenoh_python_client`

`tcnart/schema/messages/rpc.py` and `service_controller.py` mirror the IDL by
hand with pycdr2. Add the new enums and structs; append `PvtUint32 = 7`,
`PvtUint64 = 8` to `ParameterValueType`. *Not verified:* `ParameterValue` there
is a plain `typing.Union`, not a pycdr2 discriminated union, so it may not match
the IDL wire at all today.

### holohub-tcn: `operators/tcn_artekmed/tcn_ui`

`parameter_editor.py` edits a local dict (`ParameterBackend`, "Replace this
with your actual backend"); it does not speak the protocol yet. When it is
wired to Zenoh, follow the client rules above. Nothing to change for 0.9.0
itself.

### Holoscan: `ParamClient`, `tcn-params`, `libs/params_client` (holoscan-tcn-applications)

1. **`ParamClient`** (`python/holoscan_tcn/params_client.py`): `describe_all`
   (`:513`) merges `describe` + `describe_ext`; prefer `parameters/describe`
   and fall back. `set` / `set_many` / `set_described` (`:595`ff) → `set_ex`,
   mapping `ParameterSetStatus` to `SetResult`; fall back to `set`. `watch`
   (`:663`, polls at `interval`) → subscribe to `ParameterChanged` where the
   server publishes it. Add `info()` and `streams()`; `from_wire` /
   `to_wire_value` gain types 7 and 8.
2. **`tcn-params`** (`tools/params/tcn_params.py`): `cmd_list`, `cmd_set` follow
   `ParamClient`; show per-key set results.
3. **C++ `libs/params_client` and the parameter-control UI** (new, design 29):
   built against 0.9.0 from the start — discovery by liveliness `**/rpc` with
   history plus `info`, presence as fallback (*presence-only* rows); describe /
   set_ex with fallbacks; `ParameterChanged` subscription with 1 Hz polling as
   fallback.

---

## Checklist

**`libs/pcpd_zenoh`**
- [ ] liveliness token `<P>/rpc` in `ServiceController::start()` / `stop()`
- [ ] JSON `typeMap<>` for every new type, and the two new `ParameterValue` cases
- [ ] B2: no payload / no encoding is not an error
- [ ] B4: `GenericParameterReply` JSON reader checks the element

**Each ArteKMed app with parameters** (k4a, orbbec, facade/pcp daemon; sender, receiver, recorder once their handlers are live)
- [ ] `parameters/describe` registered, ranges/units/access from RTTR metadata (B6)
- [ ] `parameters/set_ex` registered, one result per key
- [ ] unsigned: `PVT_UINT*` on 0.9.0 verbs only; range-checked signed on 0.8.0 verbs (B5)
- [ ] B3: enum set checks `PVT_STRING`
- [ ] `ParameterChanged` published after every applied change
- [ ] `info` registered; `streams` where the app publishes previewable streams
- [ ] B8: `AppController` constructed with `<prefix>/<app>`

**Holoscan `libs/params_service`**
- [ ] `parameters/describe`, `parameters/set_ex`, `info`, `streams` routed and served (JSON + CDR)
- [ ] liveliness token `<P>/rpc`
- [ ] `PVT_UINT*` on 0.9.0 verbs only
- [ ] `ParameterChanged` published
- [ ] `graph` also as CDR `GraphReply`

**Every client** (slang viewer, web frontend, Python clients, `ParamClient`/`tcn-params`, C++ `params_client`/UI)
- [ ] B1 (slang viewer): IDL type ordinals
- [ ] describe → fallback `describe`; `set_ex` → fallback `set`
- [ ] `ParameterChanged` subscription, polling as fallback
- [ ] discovery by liveliness + `info`, presence as fallback
- [ ] types 7 and 8 decoded (JSON and CDR)
