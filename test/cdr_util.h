// SPDX-License-Identifier: internal
//
// Encode / decode a generated type with the encapsulation header, as the Zenoh transport carries it.
// The wire encoding is a parameter: DDS_CDR (XCDR1, encapsulation 00 01) for ROS 2 types; XCDRv2 with DELIMIT_CDR2
// (D_CDR2, 00 09) for tcn types (decisions E1, E2).
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include <fastcdr/Cdr.h>
#include <fastcdr/FastBuffer.h>

#include "check.h"

namespace tcn_test {

using eprosima::fastcdr::Cdr;
using eprosima::fastcdr::CdrVersion;
using eprosima::fastcdr::EncodingAlgorithmFlag;
using eprosima::fastcdr::FastBuffer;

// The wire encoding belongs to the package family (decisions E1-E3).
enum class Encoding {
    XCDR1,  // DDS_CDR, encapsulation 00 01: ROS 2 types
    XCDR2,  // XCDRv2 with DELIMIT_CDR2, encapsulation 00 09: tcn types
};

// The second byte of the little-endian encapsulation each encoding writes.
inline char encapsulation_id(Encoding e) { return e == Encoding::XCDR2 ? 0x09 : 0x01; }

template <typename T>
bool encode(const T& m, std::vector<char>& out, Encoding e)
{
    out.assign(1 << 20, 0);
    FastBuffer fb(out.data(), out.size());
    Cdr ser(fb, Cdr::LITTLE_ENDIANNESS, e == Encoding::XCDR2 ? CdrVersion::XCDRv2 : CdrVersion::DDS_CDR);
    try {
        // Without this an XCDRv2 Cdr writes PLAIN_CDR2 (00 07), which is not what an appendable type is (E1).
        if (e == Encoding::XCDR2) ser.set_encoding_flag(EncodingAlgorithmFlag::DELIMIT_CDR2);
        ser.serialize_encapsulation();
        ser << m;
    } catch (...) {
        return false;
    }
    out.resize(ser.get_serialized_data_length());
    return true;
}

// Reads either encoding: the reader switches on the encapsulation id, as every reader must (E3).
template <typename T>
bool decode(const std::vector<char>& in, T& m)
{
    if (in.size() < 4) return false;
    const auto id = static_cast<unsigned char>(in[1]);
    const CdrVersion version = id >= 0x06 ? CdrVersion::XCDRv2 : CdrVersion::DDS_CDR;
    std::vector<char> copy(in);
    FastBuffer fb(copy.data(), copy.size());
    Cdr de(fb, Cdr::DEFAULT_ENDIAN, version);
    try {
        de.read_encapsulation();
        de >> m;
    } catch (...) {
        return false;
    }
    return true;
}

// A default-constructed T survives encode -> decode into a fresh T (decisions E5): the payload is more than the
// 4-byte encapsulation header, and re-encoding the decoded value gives the same bytes. With compare_values, the
// generated operator== must also hold; it is off for types whose operator== is known to be wrong (fastddsgen 3.0.0
// compares members of forward-declared types by pointer, see tools/idl/roundtrip.py).
template <typename T>
void roundtrip(const char* name, bool compare_values, Encoding e)
{
    T a{};
    std::vector<char> bytes;
    bool ok = encode(a, bytes, e);
    if (!ok) std::fprintf(stderr, "encode failed: %s\n", name);
    CHECK(ok);
    CHECK(bytes.size() > 4);
    if (bytes.size() > 1 && bytes[1] != encapsulation_id(e)) std::fprintf(stderr, "wrong encapsulation: %s\n", name);
    CHECK(bytes.size() > 1 && bytes[1] == encapsulation_id(e));
    T b{};
    ok = decode(bytes, b);
    if (!ok) std::fprintf(stderr, "decode failed: %s\n", name);
    CHECK(ok);
    std::vector<char> again;
    CHECK(encode(b, again, e));
    if (again != bytes) std::fprintf(stderr, "re-encoding differs: %s\n", name);
    CHECK(again == bytes);
    if (compare_values) {
        if (!(a == b)) std::fprintf(stderr, "round trip differs: %s\n", name);
        CHECK(a == b);
    }
}

}  // namespace tcn_test

// The generated round-trip source of each package defines TCN_ENCODING from the package's family.
#define TCN_ROUNDTRIP(T) ::tcn_test::roundtrip<T>(#T, true, TCN_ENCODING)
#define TCN_ROUNDTRIP_BYTES(T) ::tcn_test::roundtrip<T>(#T, false, TCN_ENCODING)
