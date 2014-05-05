// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function testGetCharacteristic() {
  chrome.test.assertTrue(characteristic != null,
                         'Characteristic is null.');
  chrome.test.assertEq('char_id0', characteristic.instanceId);
  chrome.test.assertEq('00001211-0000-1000-8000-00805f9b34fb',
                       characteristic.uuid);
  chrome.test.assertEq(false, characteristic.isLocal);
  chrome.test.assertEq(serviceId, characteristic.service.instanceId);
  chrome.test.assertEq(4, characteristic.properties.length);
  chrome.test.assertTrue(characteristic.properties.indexOf('broadcast') > -1,
                         '\'broadcast\' not in characteristic.properties');
  chrome.test.assertTrue(characteristic.properties.indexOf('read') > -1,
                         '\'read\' not in characteristic.properties');
  chrome.test.assertTrue(characteristic.properties.indexOf('indicate') > -1,
                         '\'indicate\' not in characteristic.properties');
  chrome.test.assertTrue(
      characteristic.properties.indexOf('writeWithoutResponse') > -1,
      '\'writeWithoutResponse\' not in characteristic.properties');

  var valueBytes = new Uint8Array(characteristic.value);
  chrome.test.assertEq(5, characteristic.value.byteLength);
  chrome.test.assertEq(0x01, valueBytes[0]);
  chrome.test.assertEq(0x02, valueBytes[1]);
  chrome.test.assertEq(0x03, valueBytes[2]);
  chrome.test.assertEq(0x04, valueBytes[3]);
  chrome.test.assertEq(0x05, valueBytes[4]);

  chrome.test.succeed();
}

var serviceId = 'service_id0';
var charId = 'char_id0';
var badCharId = 'char_id1';

var characteristic = null;

// 1. Unknown characteristic instanceId.
chrome.bluetoothLowEnergy.getCharacteristic(badCharId, function (result) {
  if (result || !chrome.runtime.lastError) {
    chrome.test.fail('getCharacteristics should have failed');
  }

  // 2. Known characteristic instanceId, but the mapped device is unknown.
  chrome.bluetoothLowEnergy.getCharacteristic(charId, function (result) {
    if (result || !chrome.runtime.lastError) {
      chrome.test.fail('getCharacteristics should have failed');
    }

    // 3. Known characteristic instanceId, but the mapped service is unknown.
    chrome.bluetoothLowEnergy.getCharacteristic(charId, function (result) {
      if (result || !chrome.runtime.lastError) {
        chrome.test.fail('getCharacteristics should have failed');
      }

      // 4. Known characteristic instanceId, but the mapped service does not
      // know about the characteristic.
      chrome.bluetoothLowEnergy.getCharacteristic(charId, function (result) {
        if (result || !chrome.runtime.lastError) {
          chrome.test.fail('getCharacteristics should have failed');
        }

        // 5. Success.
        chrome.bluetoothLowEnergy.getCharacteristic(charId, function (result) {
          if (chrome.runtime.lastError) {
            chrome.test.fail(chrome.runtime.lastError.message);
          }

          characteristic = result;

          chrome.test.sendMessage('ready', function (message) {
            chrome.test.runTests([testGetCharacteristic]);
          });
        });
      });
    })
  });
});
