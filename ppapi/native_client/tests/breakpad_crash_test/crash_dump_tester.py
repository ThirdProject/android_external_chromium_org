#!/usr/bin/python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import subprocess
import sys
import tempfile
import time

script_dir = os.path.dirname(__file__)
sys.path.append(os.path.join(script_dir,
                             '../../tools/browser_tester'))

import browser_tester
import browsertester.browserlauncher

# This script extends browser_tester to check for the presence of
# Breakpad crash dumps.


# This reads a file of lines containing 'key:value' pairs.
# The file contains entries like the following:
#   plat:Win32
#   prod:Chromium
#   ptype:nacl-loader
#   rept:crash svc
def ReadDumpTxtFile(filename):
  dump_info = {}
  fh = open(filename, 'r')
  for line in fh:
    if ':' in line:
      key, value = line.rstrip().split(':', 1)
      dump_info[key] = value
  fh.close()
  return dump_info


def StartCrashService(browser_path, dumps_dir, windows_pipe_name,
                      cleanup_funcs, crash_service_exe):
  # Find crash_service.exe relative to chrome.exe.  This is a bit icky.
  browser_dir = os.path.dirname(browser_path)
  proc = subprocess.Popen([os.path.join(browser_dir, crash_service_exe),
                           '--dumps-dir=%s' % dumps_dir,
                           '--pipe-name=%s' % windows_pipe_name])

  def Cleanup():
    # Note that if the process has already exited, this will raise
    # an 'Access is denied' WindowsError exception, but
    # crash_service.exe is not supposed to do this and such
    # behaviour should make the test fail.
    proc.terminate()
    status = proc.wait()
    sys.stdout.write('crash_dump_tester: %s exited with status %s\n'
                     % (crash_service_exe, status))

  cleanup_funcs.append(Cleanup)


def GetDumpFiles(dumps_dir):
  all_files = [os.path.join(dumps_dir, dump_file)
               for dump_file in os.listdir(dumps_dir)]
  sys.stdout.write('crash_dump_tester: Found %i files\n' % len(all_files))
  for dump_file in all_files:
    sys.stdout.write('  %s\n' % dump_file)
  return [dump_file for dump_file in all_files
          if dump_file.endswith('.dmp')]


def Main(cleanup_funcs):
  parser = browser_tester.BuildArgParser()
  parser.add_option('--expected_crash_dumps', dest='expected_crash_dumps',
                    type=int, default=0,
                    help='The number of crash dumps that we should expect')
  parser.add_option('--expected_process_type_for_crash',
                    dest='expected_process_type_for_crash',
                    type=str, default='nacl-loader',
                    help='The type of Chromium process that we expect the '
                    'crash dump to be for')
  # Ideally we would just query the OS here to find out whether we are
  # running x86-32 or x86-64 Windows, but Python's win32api module
  # does not contain a wrapper for GetNativeSystemInfo(), which is
  # what NaCl uses to check this, or for IsWow64Process(), which is
  # what Chromium uses.  Instead, we just rely on the build system to
  # tell us.
  parser.add_option('--win64', dest='win64', action='store_true',
                    help='Pass this if we are running tests for x86-64 Windows')
  options, args = parser.parse_args()

  dumps_dir = tempfile.mkdtemp(prefix='nacl_crash_dump_tester_')
  def CleanUpDumpsDir():
    browsertester.browserlauncher.RemoveDirectory(dumps_dir)
  cleanup_funcs.append(CleanUpDumpsDir)

  # To get a guaranteed unique pipe name, use the base name of the
  # directory we just created.
  windows_pipe_name = r'\\.\pipe\%s_crash_service' % os.path.basename(dumps_dir)

  # This environment variable enables Breakpad crash dumping in
  # non-official builds of Chromium.
  os.environ['CHROME_HEADLESS'] = '1'
  if sys.platform == 'win32':
    # Override the default (global) Windows pipe name that Chromium will
    # use for out-of-process crash reporting.
    os.environ['CHROME_BREAKPAD_PIPE_NAME'] = windows_pipe_name
    # Launch the x86-32 crash service so that we can handle crashes in
    # the browser process.
    StartCrashService(options.browser_path, dumps_dir, windows_pipe_name,
                      cleanup_funcs, 'crash_service.exe')
    if options.win64:
      # Launch the x86-64 crash service so that we can handle crashes
      # in the NaCl loader process (nacl64.exe).
      StartCrashService(options.browser_path, dumps_dir, windows_pipe_name,
                        cleanup_funcs, 'crash_service64.exe')
    # We add a delay because there is probably a race condition:
    # crash_service.exe might not have finished doing
    # CreateNamedPipe() before NaCl does a crash dump and tries to
    # connect to that pipe.
    # TODO(mseaborn): We could change crash_service.exe to report when
    # it has successfully created the named pipe.
    time.sleep(1)
  elif sys.platform == 'darwin':
    os.environ['BREAKPAD_DUMP_LOCATION'] = dumps_dir

  result = browser_tester.Run(options.url, options)

  dmp_files = GetDumpFiles(dumps_dir)
  failed = False
  msg = ('crash_dump_tester: ERROR: Got %i crash dumps but expected %i\n' %
         (len(dmp_files), options.expected_crash_dumps))
  if len(dmp_files) != options.expected_crash_dumps:
    sys.stdout.write(msg)
    failed = True
  # On Windows, the crash dumps should come in pairs of a .dmp and
  # .txt file.
  if sys.platform == 'win32':
    for dump_file in dmp_files:
      second_file = dump_file[:-4] + '.txt'
      msg = ('crash_dump_tester: ERROR: File %r is missing a corresponding '
             '%r file\n' % (dump_file, second_file))
      if not os.path.exists(second_file):
        sys.stdout.write(msg)
        failed = True
        continue
      # Check that the crash dump comes from the NaCl process.
      dump_info = ReadDumpTxtFile(second_file)
      if 'ptype' in dump_info:
        msg = ('crash_dump_tester: ERROR: Unexpected ptype value: %r != %r\n'
               % (dump_info['ptype'], options.expected_process_type_for_crash))
        if dump_info['ptype'] != options.expected_process_type_for_crash:
          sys.stdout.write(msg)
          failed = True
      else:
        sys.stdout.write('crash_dump_tester: ERROR: Missing ptype field\n')
        failed = True
    # TODO(mseaborn): Ideally we would also check that a backtrace
    # containing an expected function name can be extracted from the
    # crash dump.

  if failed:
    sys.stdout.write('crash_dump_tester: FAILED\n')
    result = 1
  else:
    sys.stdout.write('crash_dump_tester: PASSED\n')

  return result


def MainWrapper():
  cleanup_funcs = []
  try:
    return Main(cleanup_funcs)
  finally:
    for func in cleanup_funcs:
      func()


if __name__ == '__main__':
  sys.exit(MainWrapper())
