// SPDX-License-Identifier: internal
//
// Golden byte files: fixed values, encoded, compared byte for byte with test/golden/<case>.hex (space-separated
// lower-case hex). The files are the contract every non-fastcdr codec (Rust, TypeScript, C#, Python) tests against.
//
// TCN_SCHEMA_UPDATE_GOLDEN=1 rewrites the files instead of comparing. Do that only for a deliberate wire change, and
// review the diff: the encodings change exactly when the decisions say (0.11 plan 4: tcn types move to XCDR2).
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "cdr_util.h"

#include "std_msgs/msg/Header.h"
#include "std_msgs/msg/HeaderCdrAux.hpp"
#include "tcnart_msgs/msg/PoseTracking.h"
#include "tcnart_msgs/msg/PoseTrackingCdrAux.hpp"
#include "sqe_msgs/msg/Graph.h"
#include "sqe_msgs/msg/GraphCdrAux.hpp"
#include "evo_v1/msg/EvolutionV1.h"
#include "evo_v1/msg/EvolutionV1CdrAux.hpp"
#include "evo_v2/msg/EvolutionV2.h"
#include "evo_v2/msg/EvolutionV2CdrAux.hpp"

#ifndef TCN_GOLDEN_DIR
#error "TCN_GOLDEN_DIR must name the directory of the .hex files"
#endif

static std::string to_hex(const std::vector<char>& b)
{
    std::string s;
    char buf[4];
    for (size_t i = 0; i < b.size(); ++i) {
        std::snprintf(buf, sizeof buf, "%02x", static_cast<unsigned char>(b[i]));
        if (i) s += ' ';
        s += buf;
    }
    return s;
}

using tcn_test::Encoding;

static std::vector<char> from_hex(const std::string& hex)
{
    std::vector<char> b;
    std::istringstream in(hex);
    unsigned x;
    while (in >> std::hex >> x) b.push_back(static_cast<char>(x));
    return b;
}

static std::vector<char> golden_bytes(const char* name)
{
    std::ifstream in(std::string(TCN_GOLDEN_DIR) + "/" + name + ".hex");
    std::stringstream s;
    s << in.rdbuf();
    return from_hex(s.str());
}

// Encodes `value` with `e` and compares with test/golden/<name>.hex; returns the bytes.
template <typename T>
static std::vector<char> golden(const char* name, const T& value, Encoding e)
{
    std::vector<char> bytes;
    CHECK(tcn_test::encode(value, bytes, e));
    CHECK(bytes.size() > 1 && bytes[1] == tcn_test::encapsulation_id(e));
    // Only ROS 2 types (std_msgs here) are XCDR1; every other golden must prove XCDR2 (decisions E1, E2).
    CHECK((std::string(name).rfind("std_msgs_", 0) == 0) == (e == Encoding::XCDR1));
    const std::string path = std::string(TCN_GOLDEN_DIR) + "/" + name + ".hex";
    const std::string hex = to_hex(bytes);
    const char* update = std::getenv("TCN_SCHEMA_UPDATE_GOLDEN");
    if (update && std::string(update) == "1") {
        std::ofstream(path) << hex << "\n";
        std::printf("updated %s\n", path.c_str());
    } else {
        std::ifstream in(path);
        std::stringstream want;
        want << in.rdbuf();
        std::string w = want.str();
        while (!w.empty() && (w.back() == '\n' || w.back() == ' ')) w.pop_back();
        if (w != hex) std::fprintf(stderr, "%s:\n  want %s\n  got  %s\n", name, w.c_str(), hex.c_str());
        CHECK(w == hex);
    }
    T back{};
    CHECK(tcn_test::decode(bytes, back));
    CHECK(back == value);
    return bytes;
}

// Type evolution across the two versions of evo::Thing (decisions E5, E8), against the golden bytes.
static void evolution()
{
    evo_v1::msg::Thing v1;
    v1.a(5);
    v1.b("ab");
    golden("evo_v1_Thing", v1, Encoding::XCDR2);

    evo_v2::msg::Thing v2;
    v2.a(7);
    v2.b("hi");
    v2.c(1.5);
    v2.d(0x1122334455667788LL);
    golden("evo_v2_Thing", v2, Encoding::XCDR2);

    // Newer bytes, older reader: the appended members are skipped by the DHEADER.
    evo_v1::msg::Thing old_reader{};
    CHECK(tcn_test::decode(golden_bytes("evo_v2_Thing"), old_reader));
    CHECK(old_reader.a() == 7);
    CHECK(old_reader.b() == "hi");

    // Older bytes, newer reader decoding into a fresh object: c absent, d = 0.
    evo_v2::msg::Thing new_reader{};
    CHECK(tcn_test::decode(golden_bytes("evo_v1_Thing"), new_reader));
    CHECK(new_reader.a() == 5);
    CHECK(new_reader.b() == "ab");
    CHECK(!new_reader.c().has_value());
    CHECK(new_reader.d() == 0);

    // Grown structs inside a container, followed by more data: the old reader must skip by every DHEADER.
    evo_v2::msg::Holder h2;
    evo_v2::msg::Thing t2b = v2;
    t2b.a(8);
    t2b.b("yo");
    h2.things({v2, t2b});
    h2.tail(-1);
    golden("evo_v2_Holder", h2, Encoding::XCDR2);
    evo_v1::msg::Holder old_holder{};
    const auto bytes2 = golden_bytes("evo_v2_Holder");
    CHECK(tcn_test::decode(bytes2, old_holder));
    CHECK(tcn_test::consumed(bytes2, old_holder) == bytes2.size());
    CHECK(old_holder.things().size() == 2);
    CHECK(old_holder.things().size() == 2 && old_holder.things()[1].a() == 8 && old_holder.things()[1].b() == "yo");
    CHECK(old_holder.tail() == -1);

    evo_v1::msg::Holder h1;
    h1.things({v1, v1});
    h1.tail(42);
    golden("evo_v1_Holder", h1, Encoding::XCDR2);
    evo_v2::msg::Holder new_holder{};
    const auto bytes1 = golden_bytes("evo_v1_Holder");
    CHECK(tcn_test::decode(bytes1, new_holder));
    CHECK(new_holder.things().size() == 2 && !new_holder.things()[1].c().has_value() && new_holder.things()[1].d() == 0);
    CHECK(new_holder.tail() == 42);
}

static std_msgs::msg::Header header()
{
    std_msgs::msg::Header h;
    h.stamp().sec(1);
    h.stamp().nanosec(2);
    h.frame_id("map");
    return h;
}

int main()
{
    golden("std_msgs_Header", header(), Encoding::XCDR1);

    tcnart_msgs::msg::TargetTrackingMessage tt;
    tt.header(header());
    tcn_types::msg::Target target;
    target.target_id(7);
    target.pose().translation().x(1);
    target.pose().translation().y(2);
    target.pose().translation().z(3);
    target.pose().rotation().w(1);
    target.is_tracked(true);
    tt.targets().push_back(target);
    golden("tcnart_msgs_TargetTrackingMessage", tt, Encoding::XCDR2);

    sqe_msgs::msg::SRGraph g;
    sqe_msgs::msg::SRNode node;
    node.name("a");
    g.nodes().push_back(node);
    sqe_msgs::msg::SREdge edge;
    edge.source("a");
    edge.target("b");
    tcn_types::msg::RigidTransform identity;
    identity.rotation().w(1);
    edge.transform().pose6d(identity);
    g.edges().push_back(edge);
    golden("sqe_msgs_SRGraph", g, Encoding::XCDR2);

    evolution();

    return tcn_test_exit("golden");
}
