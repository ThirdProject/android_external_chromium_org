# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
{
  'action_name': 'repack_resources',
  'variables': {
    'pak_inputs': [
      '<(grit_out_dir)/component_extension_resources.pak',
      '<(grit_out_dir)/devtools_frontend_resources.pak',
      '<(grit_out_dir)/devtools_resources.pak',
      '<(grit_out_dir)/net_internals_resources.pak',
      '<(grit_out_dir)/options_resources.pak',
      '<(grit_out_dir)/shared_resources.pak',
      '<(grit_out_dir)/sync_internals_resources.pak',
      '<(grit_out_dir)/workers_resources.pak',
    ],
    'conditions': [
      ['OS != "mac"', {
        'pak_inputs': [
          '<(grit_out_dir)/quota_internals_resources.pak',
        ],
      }],
    ],
  },
  'inputs': [
    '<(repack_path)',
    '<@(pak_inputs)',
  ],
  'outputs': [
    '<(INTERMEDIATE_DIR)/repack/resources.pak',
  ],
  'action': ['python', '<(repack_path)', '<@(_outputs)', '<@(pak_inputs)'],
}
