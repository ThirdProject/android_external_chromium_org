#!/usr/bin/env python
#
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Process Android library resources to generate R.java and crunched images."""

import optparse
import os
import shlex

from util import build_utils

def ParseArgs():
  """Parses command line options.

  Returns:
    An options object as from optparse.OptionsParser.parse_args()
  """
  parser = optparse.OptionParser()
  parser.add_option('--android-sdk', help='path to the Android SDK folder')
  parser.add_option('--android-sdk-tools',
                    help='path to the Android SDK build tools folder')
  parser.add_option('--R-dir', help='directory to hold generated R.java')
  parser.add_option('--res-dirs',
                    help='directories containing resources to be packaged')
  parser.add_option('--crunch-input-dir',
                    help='directory containing images to be crunched')
  parser.add_option('--crunch-output-dir',
                    help='directory to hold crunched resources')
  parser.add_option('--non-constant-id', action='store_true')
  parser.add_option('--custom-package', help='Java package for R.java')
  parser.add_option('--android-manifest', help='AndroidManifest.xml path')
  parser.add_option('--stamp', help='File to touch on success')

  # This is part of a temporary fix for crbug.com/177552.
  # TODO(newt): remove this once crbug.com/177552 is fixed in ninja.
  parser.add_option('--ignore', help='this argument is ignored')
  (options, args) = parser.parse_args()

  if args:
    parser.error('No positional arguments should be given.')

  # Check that required options have been provided.
  required_options = ('android_sdk', 'android_sdk_tools', 'R_dir',
                      'res_dirs', 'crunch_input_dir', 'crunch_output_dir')
  build_utils.CheckOptions(options, parser, required=required_options)

  return options


def main():
  options = ParseArgs()
  android_jar = os.path.join(options.android_sdk, 'android.jar')
  aapt = os.path.join(options.android_sdk_tools, 'aapt')

  build_utils.MakeDirectory(options.R_dir)

  # Generate R.java. This R.java contains non-final constants and is used only
  # while compiling the library jar (e.g. chromium_content.jar). When building
  # an apk, a new R.java file with the correct resource -> ID mappings will be
  # generated by merging the resources from all libraries and the main apk
  # project.
  package_command = [aapt,
                     'package',
                     '-m',
                     '-M', options.android_manifest,
                     '--auto-add-overlay',
                     '-I', android_jar,
                     '--output-text-symbols', options.R_dir,
                     '-J', options.R_dir]
  res_dirs = shlex.split(options.res_dirs)
  for res_dir in res_dirs:
    package_command += ['-S', res_dir]
  if options.non_constant_id:
    package_command.append('--non-constant-id')
  if options.custom_package:
    package_command += ['--custom-package', options.custom_package]
  build_utils.CheckCallDie(package_command)

  # Crunch image resources. This shrinks png files and is necessary for 9-patch
  # images to display correctly.
  build_utils.MakeDirectory(options.crunch_output_dir)

  aapt_cmd = [aapt,
              'crunch',
              '-S', options.crunch_input_dir,
              '-C', options.crunch_output_dir]
  build_utils.CheckCallDie(aapt_cmd, suppress_output=True)

  if options.stamp:
    build_utils.Touch(options.stamp)


if __name__ == '__main__':
  main()
