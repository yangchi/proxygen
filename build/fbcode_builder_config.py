#!/usr/bin/env python
# Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved
from __future__ import absolute_import, division, print_function, unicode_literals

import specs.fizz as fizz
import specs.folly as folly
import specs.mvfst as mvfst
import specs.proxygen_quic as proxygen_quic
import specs.sodium as sodium
import specs.wangle as wangle
import specs.zstd as zstd
from shell_quoting import ShellQuoted


"fbcode_builder steps to build & test Proxygen"


def fbcode_builder_spec(builder):
    return {
        "depends_on": [folly, wangle, fizz, sodium, zstd, mvfst, proxygen_quic],
        "steps": [
            # Tests for the full build with no QUIC/HTTP3
            # Proxygen is the last step, so we are still in its working dir.
            builder.step(
                "Run proxygen tests",
                [builder.run(ShellQuoted("env CTEST_OUTPUT_ON_FAILURE=1 make test"))],
            )
        ],
    }


config = {
    "github_project": "facebook/proxygen",
    "fbcode_builder_spec": fbcode_builder_spec,
}
