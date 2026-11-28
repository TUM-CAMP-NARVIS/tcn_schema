// SPDX-License-Identifier: internal
//
// The generated joint-set arrays (tcn/joint_sets.h, from tcnart_msgs/msg/joint_sets.yaml) agree with the generated
// IDL enum tcnart_msgs::msg::JointSet, and find() refuses an unknown (set, version) pair.
#include <cstring>

#include "check.h"
#include "tcn/joint_sets.h"
#include "tcnart_msgs/msg/JointSet.h"

namespace js = tcn::joint_sets;
namespace msg = tcnart_msgs::msg;

#define SAME_VALUE(cpp, idl) static_assert(static_cast<int>(js::JointSet::cpp) == static_cast<int>(msg::idl), #cpp)
SAME_VALUE(Unknown, JOINT_SET_UNKNOWN);
SAME_VALUE(Coco17, JOINT_SET_COCO17);
SAME_VALUE(Halpe26, JOINT_SET_HALPE26);
SAME_VALUE(TcnLite, JOINT_SET_TCN_LITE);
SAME_VALUE(TcnBody, JOINT_SET_TCN_BODY);
SAME_VALUE(Smpl24, JOINT_SET_SMPL_24);
SAME_VALUE(Smplx55, JOINT_SET_SMPLX_55);
SAME_VALUE(Smplh52, JOINT_SET_SMPLH_52);

static_assert(js::find(js::JointSet::TcnBody, 1) != nullptr, "find is usable in constant expressions");

int main()
{
    const js::JointSetInfo* body = js::find(js::JointSet::TcnBody, 1);
    CHECK(body != nullptr);
    if (body) {
        CHECK(body->joint_count == 20);
        CHECK(body->bone_count == 19);
        CHECK(!body->positions_only);
        CHECK(std::strcmp(body->joint_names[0], "pelvis") == 0);
        CHECK(body->parents[0] == -1);
        CHECK(body->bones[5].from == 3 && body->bones[5].to == 6);  // neck -> left_shoulder, +x
        CHECK(body->bones[5].rest_dir[0] == 1.0f && body->bones[5].rest_dir[1] == 0.0f);
    }
    CHECK(js::find(js::JointSet::TcnBody, 2) == nullptr);
    CHECK(js::find(js::JointSet::Unknown, 1) == nullptr);
    CHECK(js::find(static_cast<js::JointSet>(99), 1) == nullptr);

    const js::JointSetInfo* halpe = js::find(js::JointSet::Halpe26, 1);
    CHECK(halpe != nullptr && halpe->positions_only);
    if (halpe) {
        CHECK(halpe->joint_count == 26 && halpe->bone_count == 26);
        CHECK(halpe->parents[17] == -1);
        CHECK(halpe->bones[0].rest_dir[0] != halpe->bones[0].rest_dir[0]);  // NaN: no rest direction
    }

    const js::JointSetInfo* lite = js::find(js::JointSet::TcnLite, 1);
    CHECK(lite != nullptr && lite->parents[0] == 1);  // head -> chest

    const js::JointSetInfo* smplx = js::find(js::JointSet::Smplx55, 1);
    CHECK(smplx != nullptr && smplx->joint_count == 55 && smplx->bone_count == 0 && smplx->bones == nullptr);
    if (smplx) {
        CHECK(std::strcmp(smplx->joint_names[25], "left_index1") == 0 && smplx->parents[25] == 20);
    }
    return tcn_test_exit("joint_sets");
}
