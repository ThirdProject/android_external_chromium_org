// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * TestFixture for SUID Sandbox testing.
 * @extends {testing.Test}
 * @constructor
 */
function SandboxStatusUITest() {}

SandboxStatusUITest.prototype = {
  __proto__: testing.Test.prototype,
  /**
   * Browse to the options page & call our preLoad().
   */
  browsePreload: 'chrome://sandbox',

};

// This test is for Linux only.
// PLEASE READ:
// - If failures of this test are a problem on a bot under your care,
//   the proper way to address such failures is to install the SUID
//   sandbox. See:
//     http://code.google.com/p/chromium/wiki/LinuxSUIDSandboxDevelopment
// - PLEASE DO NOT GLOBALLY DISABLE THIS TEST.
// SUID sandbox is currently incompatible with AddressSanitizer,
// see http://crbug.com/137653.
GEN('#if defined(OS_LINUX) && !defined(ADDRESS_SANITIZER)');
GEN('# define MAYBE_testSUIDSandboxEnabled \\');
GEN('     testSUIDSandboxEnabled');
GEN('#else');
GEN('# define MAYBE_testSUIDSandboxEnabled \\');
GEN('     DISABLED_testSUIDSandboxEnabled');
GEN('#endif');

/**
 * Test if the SUID sandbox is enabled.
 */
TEST_F('SandboxStatusUITest', 'MAYBE_testSUIDSandboxEnabled', function() {
    var suidyesstring = 'SUID Sandbox\tYes';
    var suidnostring = 'SUID Sandbox\tNo';
    var suidyes = document.body.innerText.match(suidyesstring);
    var suidno = document.body.innerText.match(suidnostring);

    expectEquals(null, suidno);
    assertFalse(suidyes === null);
    expectEquals(suidyesstring, suidyes[0]);
});

// The seccomp-bpf sandbox is also not compatible with ASAN.
// The Chrome OS waterfall bot doesn't support BPF: crbug.com/237041.
GEN('#if defined(OS_LINUX) && !defined(ADDRESS_SANITIZER) && \\');
GEN('    !defined(OS_CHROMEOS)');
GEN('# define MAYBE_testBPFSandboxEnabled \\');
GEN('     testBPFSandboxEnabled');
GEN('#else');
GEN('# define MAYBE_testBPFSandboxEnabled \\');
GEN('     DISABLED_testBPFSandboxEnabled');
GEN('#endif');

/**
 * Test if the seccomp-bpf sandbox is enabled.
 */
TEST_F('SandboxStatusUITest', 'MAYBE_testBPFSandboxEnabled', function() {
    var bpfyesstring = 'Seccomp-BPF sandbox\tYes';
    var bpfnostring = 'Seccomp-BPF sandbox\tNo';
    var bpfyes = document.body.innerText.match(bpfyesstring);
    var bpfno = document.body.innerText.match(bpfnostring);

    expectEquals(null, bpfno);
    assertFalse(bpfyes === null);
    expectEquals(bpfyesstring, bpfyes[0]);
});
