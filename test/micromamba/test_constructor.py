import json
import os
import shutil
import subprocess
from pathlib import Path

import pytest

from .helpers import install, create, get_umamba, random_string

# CONDA_SAFETY_CHECKS=disabled \
# CONDA_EXTRA_SAFETY_CHECKS=no \
# CONDA_CHANNELS=__CHANNELS__ \
# CONDA_PKGS_DIRS="$PREFIX/pkgs" \
# "$CONDA_EXEC" install --offline --file "$PREFIX/pkgs/env.txt" -yp "$PREFIX" || exit 1

class TestConstructor:
    spec_files_location = os.path.expanduser(
        os.path.join("~", "mamba_spec_files_test_" + random_string())
    )

    @classmethod
    def setup_class(cls):
        os.makedirs(cls.spec_files_location, exist_ok=True)

    def test_config(self):
        spec_file = os.path.join(self.spec_files_location, random_string() + ".txt")
        with open(spec_file, "w") as f:
            f.write("\n".join(["xtensor", "xsimd"]))

        res = install("--offline", "--print-context-only")
        assert(res["offline"] == True)
        res = install("--offline", "--file", spec_file, "--print-context-only")
        print(res)

        os.environ["CONDA_PKGS_DIRS"] = "/some/weird/dir"
        # res = install("--offline", "--file", spec_file, "--print-context-only")
        # assert(res["pkgs_dirs"] == ["/some/weird/dir"])
        # res = install("--offline", "--file", spec_file, "-yp", "~/mamba_spec_file_prefixtest", "--print-context-only")
        # print(res)