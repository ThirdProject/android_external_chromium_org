# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# TODO(jochen): make this a .gypi file and include it in content.gyp, once
# content.gyp doesn't depend on gyp files from chrome/ anymore:
# http://crbug.com/10394
{
  'variables': {
    'grit_out_dir': '<(SHARED_INTERMEDIATE_DIR)/content',
  },
  'targets': [
    {
      'target_name': 'content_resources',
      'type': 'none',
      'actions': [
        {
          'action_name': 'content_resources',
          'variables': {
            'grit_grd_file': 'content_resources.grd',
          },
          'includes': [ '../build/grit_action.gypi' ],
        },
      ],
      'includes': [ '../build/grit_target.gypi' ],
    },
  ],
}
