from conan import ConanFile
from conan.tools.files import apply_conandata_patches, collect_libs, copy, export_conandata_patches, get, rename, replace_in_file, rmdir, save
from conan.tools.env import VirtualRunEnv
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout, CMakeDeps
import os

class TcnSchemaConan(ConanFile):
    python_requires = "camp_common/0.5@camposs/stable"
    python_requires_extend = "camp_common.CampCMakeBase"

    name = "tcn_schema"
    version = "0.0.1"

    description = "HL2Comm via Zenoh with Ros2 Serialization Compatible Schemata"
    url = "https://github.com/TUM-CAMP-NARVIS/tcn_schema"
    license = "internal"

    settings = "os", "compiler", "build_type", "arch"

    options = {
        "shared": [True, False],
        "with_dds": [True, False],
    }
    default_options = {
        "shared" : False,
        "with_dds": False,
    }

    # all sources are deployed with the package
    exports_sources = "3rdparty/*", "cmake/*", "src/*", "CMakeLists.txt"

    def build_requirements(self):
        self.tool_requires("fast-dds-gen/2.4.0@camposs/stable")
        self.tool_requires("zulu-openjdk/11.0.15")

    def requirements(self):
        self.requires("fast-cdr/1.0.27")
        if self.options.with_dds:
            self.requires("fast-dds/2.11.1")

    def _configure_toolchain(self, tc):
        dep = self.dependencies.build["fast-dds-gen"]
        fpath = os.path.join(dep.package_folder, "share", "fastddsgen", "java")
        self.output.info("Using FASTDDS generator path: {0}".format(fpath))
        tc.variables["FASTDDS_GEN_JAR_PATH"] = str(fpath)

    def build(self):
        # overwritten since generator needs to be able to find its shared lib if build with_shared
        with VirtualRunEnv(self).vars().apply():
            cmake = CMake(self)
            self._before_configure()
            cmake.configure()
            self._before_build(cmake)
            cmake.build()
            self._after_build()


    def package_info(self):
        self.cpp_info.names["cmake_find_package"] = "tcn_schema"
        self.cpp_info.names["cmake_find_package_multi"] = "tcn_schema"

        def dds_dep():
            if self.options.with_dds:
                return ["fast-dds::fast-dds"]
            else:
                return ["fast-cdr::fast-cdr"]

        # ROS2 default messages
        self.cpp_info.components["std_msgs"].names["cmake_find_package"] = "std_msgs"
        self.cpp_info.components["std_msgs"].libs = ["tcn_schema_dds_std"]
        self.cpp_info.components["std_msgs"].requires = dds_dep() + []

        self.cpp_info.components["geometry_msgs"].names["cmake_find_package"] = "geometry_msgs"
        self.cpp_info.components["geometry_msgs"].libs = ["tcn_schema_dds_geometry"]
        self.cpp_info.components["geometry_msgs"].defines = []
        self.cpp_info.components["geometry_msgs"].requires = dds_dep() + ["std_msgs"]

        self.cpp_info.components["sensor_msgs"].names["cmake_find_package"] = "sensor_msgs"
        self.cpp_info.components["sensor_msgs"].libs = ["tcn_schema_dds_sensor"]
        self.cpp_info.components["sensor_msgs"].defines = []
        self.cpp_info.components["sensor_msgs"].requires = dds_dep() + ["std_msgs", "geometry_msgs"]

        self.cpp_info.components["shape_msgs"].names["cmake_find_package"] = "shape_msgs"
        self.cpp_info.components["shape_msgs"].libs = ["tcn_schema_dds_shape"]
        self.cpp_info.components["shape_msgs"].defines = []
        self.cpp_info.components["shape_msgs"].requires = dds_dep() + ["std_msgs", "geometry_msgs"]

        self.cpp_info.components["tf2_msgs"].names["cmake_find_package"] = "tf2_msgs"
        self.cpp_info.components["tf2_msgs"].libs = ["tcn_schema_dds_tf2"]
        self.cpp_info.components["tf2_msgs"].defines = []
        self.cpp_info.components["tf2_msgs"].requires = dds_dep() + ["std_msgs", "geometry_msgs"]

        self.cpp_info.components["rosgraph_msgs"].names["cmake_find_package"] = "rosgraph_msgs"
        self.cpp_info.components["rosgraph_msgs"].libs = ["tcn_schema_dds_rosgraph"]
        self.cpp_info.components["rosgraph_msgs"].defines = []
        self.cpp_info.components["rosgraph_msgs"].requires = dds_dep() + ["std_msgs"]

        self.cpp_info.components["statistics_msgs"].names["cmake_find_package"] = "statistics_msgs"
        self.cpp_info.components["statistics_msgs"].libs = ["tcn_schema_dds_statistics"]
        self.cpp_info.components["statistics_msgs"].defines = []
        self.cpp_info.components["statistics_msgs"].requires = dds_dep() + []

        self.cpp_info.components["unique_identifier_msgs"].names["cmake_find_package"] = "unique_identifier_msgs"
        self.cpp_info.components["unique_identifier_msgs"].libs = ["tcn_schema_dds_unique_identifier"]
        self.cpp_info.components["unique_identifier_msgs"].defines = []
        self.cpp_info.components["unique_identifier_msgs"].requires = dds_dep() + []


        # Custom messages
        self.cpp_info.components["pcpd_msgs"].names["cmake_find_package"] = "pcpd_msgs"
        self.cpp_info.components["pcpd_msgs"].libs = ["tcn_schema_dds_pcpd"]
        self.cpp_info.components["pcpd_msgs"].defines = []
        self.cpp_info.components["pcpd_msgs"].requires = dds_dep() + ["std_msgs", "geometry_msgs"]

        self.cpp_info.components["device_hl2_msgs"].names["cmake_find_package"] = "device_hl2_msgs"
        self.cpp_info.components["device_hl2_msgs"].libs = ["tcn_schema_dds_device_hl2"]
        self.cpp_info.components["device_hl2_msgs"].defines = []
        self.cpp_info.components["device_hl2_msgs"].requires = dds_dep() + ["std_msgs", "geometry_msgs"]

