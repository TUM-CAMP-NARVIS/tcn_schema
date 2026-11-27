"""The ROS 2 package tiers (decisions §5 R-D2; housekeeping proposal §3.4 R4)."""
TIERS = {
    "base": ["builtin_interfaces", "std_msgs", "geometry_msgs", "sensor_msgs"],
    "core": ["nav_msgs", "trajectory_msgs", "diagnostic_msgs", "visualization_msgs", "shape_msgs", "std_srvs",
             "stereo_msgs", "unique_identifier_msgs", "action_msgs", "service_msgs", "rcl_interfaces",
             "lifecycle_msgs", "rosgraph_msgs", "statistics_msgs", "tf2_msgs", "type_description_interfaces"],
    "manipulation": ["control_msgs", "controller_manager_msgs", "moveit_msgs", "object_recognition_msgs",
                     "octomap_msgs"],
    "navigation": ["nav2_msgs", "geographic_msgs"],
    "perception": ["vision_msgs", "pcl_msgs"],
}
DEFAULT_TIERS = ["base", "core"]


def tier_of() -> dict:
    return {pkg: tier for tier, pkgs in TIERS.items() for pkg in pkgs}
