// SPDX-License-Identifier: internal
//
// Encode / decode a generated type with the encapsulation header, as the Zenoh transport carries it.
// The wire encoding is a parameter: DDS_CDR (XCDR1, encapsulation 00 01) up to 0.10.x and for ROS 2 types; XCDRv2
// (D_CDR2, 00 09) for tcn types from 0.11 (decisions E1, E2).
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
using eprosima::fastcdr::FastBuffer;

template <typename T>
bool encode(const T& m, std::vector<char>& out, CdrVersion version = CdrVersion::DDS_CDR)
{
    out.assign(1 << 20, 0);
    FastBuffer fb(out.data(), out.size());
    Cdr ser(fb, Cdr::LITTLE_ENDIANNESS, version);
    try {
        ser.serialize_encapsulation();
        ser << m;
    } catch (...) {
        return false;
    }
    out.resize(ser.get_serialized_data_length());
    return true;
}

template <typename T>
bool decode(const std::vector<char>& in, T& m, CdrVersion version = CdrVersion::DDS_CDR)
{
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
void roundtrip(const char* name, bool compare_values)
{
    T a{};
    std::vector<char> bytes;
    bool ok = encode(a, bytes);
    if (!ok) std::fprintf(stderr, "encode failed: %s\n", name);
    CHECK(ok);
    CHECK(bytes.size() > 4);
    T b{};
    ok = decode(bytes, b);
    if (!ok) std::fprintf(stderr, "decode failed: %s\n", name);
    CHECK(ok);
    std::vector<char> again;
    CHECK(encode(b, again));
    if (again != bytes) std::fprintf(stderr, "re-encoding differs: %s\n", name);
    CHECK(again == bytes);
    if (compare_values) {
        if (!(a == b)) std::fprintf(stderr, "round trip differs: %s\n", name);
        CHECK(a == b);
    }
}

}  // namespace tcn_test

#define TCN_ROUNDTRIP(T) ::tcn_test::roundtrip<T>(#T, true)
#define TCN_ROUNDTRIP_BYTES(T) ::tcn_test::roundtrip<T>(#T, false)
