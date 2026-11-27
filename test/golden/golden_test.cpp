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

template <typename T>
static void golden(const char* name, const T& value)
{
    std::vector<char> bytes;
    CHECK(tcn_test::encode(value, bytes));
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
    golden("std_msgs_Header", header());

    tcnart_msgs::msg::TargetTrackingMessage tt;
    tt.header(header());
    tcn_types::msg::Target target;
    target.target_id(7);
    target.pose().translation().x(1);
    target.pose().translation().y(2);
    target.pose().translation().z(3);
    target.pose().rotation().w(1);
    target.quality(1.0f);
    target.is_tracked(true);
    tt.targets().push_back(target);
    golden("tcnart_msgs_TargetTrackingMessage", tt);

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
    golden("sqe_msgs_SRGraph", g);

    return tcn_test_exit("golden");
}
