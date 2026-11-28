from conan import ConanFile
from conan.tools.files import apply_conandata_patches, collect_libs, copy, export_conandata_patches, get, rename, replace_in_file, rmdir, save
from conan.tools.env import VirtualRunEnv
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout, CMakeDeps
from conan.errors import ConanException
import json
import os

class TcnSchemaConan(ConanFile):
    python_requires = "camp_common/[>=0.5 <1.0]@camposs/stable"
    python_requires_extend = "camp_common.CampCMakeBase"

    name = "tcn_schema"
    version = "0.11.0-rc1"

    description = "HL2Comm via Zenoh with Ros2 Serialization Compatible Schemata"
    url = "https://github.com/TUM-CAMP-NARVIS/tcn_schema"
    license = "internal"

    settings = "os", "compiler", "build_type", "arch"

    options = {
        "shared": [True, False],
        "with_dds": [True, False],
        # ROS 2 interface tiers to build, ';'-separated: base;core (default), manipulation, navigation, perception
        "ros2_tiers": ["ANY"],
    }
    default_options = {
        "shared" : False,
        "with_dds": False,
        "ros2_tiers": "base;core",
    }

    # all sources are deployed with the package
    #exports_sources = "3rdparty/*", "cmake/*", "src/*", "CMakeLists.txt"

    def export_sources(self):
        copy(self, "3rdparty/*", src=self.recipe_folder, dst=self.export_sources_folder, keep_path=True)
        copy(self, "cmake/*", src=self.recipe_folder, dst=self.export_sources_folder, keep_path=True)
        copy(self, "src/*", src=self.recipe_folder, dst=self.export_sources_folder, keep_path=True)
        copy(self, "tools/*.py", src=self.recipe_folder, dst=self.export_sources_folder, keep_path=True)
        copy(self, "tools/idl/*.py", src=self.recipe_folder, dst=self.export_sources_folder, keep_path=True)
        copy(self, "CMakeLists.txt", src=self.recipe_folder, dst=self.export_sources_folder, keep_path=True)

    def build_requirements(self):
        self.tool_requires("fast-dds-gen/3.0.0@camposs/stable")
        self.tool_requires("zulu-openjdk/11.0.15")

    def requirements(self):
        # The generated headers include fastcdr headers, so consumers need them on their include path.
        self.requires("fast-cdr/2.0.0@camposs/stable", transitive_headers=True)
        if self.options.with_dds:
            self.requires("fast-dds/2.11.1", transitive_headers=True)

    def configure(self):
        if self.options.shared:
            self.options["fast-cdr"].shared = True
            if self.options.with_dds:
                self.options["fast-dds"].shared=True

    def _configure_toolchain(self, tc):
        dep = self.dependencies.build["fast-dds-gen"]
        fpath = os.path.join(dep.package_folder, "share", "fastddsgen", "java")
        self.output.info("Using FASTDDS generator path: {0}".format(fpath))
        gen_path = str(fpath)
        if self.settings.os == "Windows" or self.settings.os == "WindowsStore":
            gen_path = gen_path.replace("\\", "\\\\")
        tc.variables["FASTDDS_GEN_JAR_PATH"] = gen_path
        tc.cache_variables["TCN_SCHEMA_ROS2_TIERS"] = ";".join(t.strip() for t in str(self.options.ros2_tiers).split(";"))

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

        dds_dep = ["fast-dds::fast-dds"] if self.options.with_dds else ["fast-cdr::fast-cdr"]

        # One component per entry of the package table (cmake/tcn_packages.json, installed as
        # share/tcn_schema/packages.json); CMake builds from the same table. `header` entries (the SQE contract
        # library) have no library.
        # Installed package: share/tcn_schema/packages.json. Editable mode (package_folder is the source tree): the
        # source table cmake/tcn_packages.json.
        candidates = [os.path.join(self.package_folder, "share", "tcn_schema", "packages.json"),
                      os.path.join(self.package_folder, "cmake", "tcn_packages.json"),
                      os.path.join(self.recipe_folder, "cmake", "tcn_packages.json")]
        table = next((c for c in candidates if os.path.isfile(c)), None)
        if table is None:
            raise ConanException("tcn_schema: package table not found in " + ", ".join(candidates))
        with open(table, encoding="utf-8") as f:
            packages = json.load(f)["packages"]
        tiers = [t.strip() for t in str(self.options.ros2_tiers).split(";")]
        for p in packages:
            # A ROS 2 tier that was not selected (option ros2_tiers). The installed table says so ("built"); the
            # source table used in editable mode does not, so the option decides there.
            if "built" in p and not p["built"]:
                continue
            if "built" not in p and p["family"] == "ros2" and p.get("tier") not in tiers:
                continue
            c = self.cpp_info.components[p["name"]]
            c.names["cmake_find_package"] = p["name"]
            c.defines = []
            if p["family"] == "header":
                c.libs = []
                c.requires = list(p["deps"])
            else:
                c.libs = ["tcn_schema_" + p["name"]]
                c.requires = dds_dep + list(p["deps"])
