# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
from PIL import Image
import sets
import sys
import unittest

import cloud_bucket
import image_tools
import ispy_utils
import mock_cloud_bucket


class ISpyUtilsUnitTest(unittest.TestCase):

  def setUp(self):
    # Set up structures that will be reused throughout testing.
    self.bucket = mock_cloud_bucket.MockCloudBucket()
    self.ispy_utils = ispy_utils.ISpyUtils(self.bucket)
    self.white = Image.new('RGBA', (25, 25), (255, 255, 255, 255))
    self.red = Image.new('RGBA', (25, 25), (255, 0, 0, 255))
    self.black = Image.new('RGBA', (25, 25), (0, 0, 0, 255))
    self.masked = Image.new('RGBA', (25, 25), (210, 0, 0, 255))

  def testUploadImage(self):
    self.bucket.Reset()
    # Upload some images to the datastore.
    self.ispy_utils.UploadImage('path/to/white.png', self.white)
    self.ispy_utils.UploadImage('path/to/black.png', self.black)
    self.ispy_utils.UploadImage('path/to/red.png', self.red)
    # Confirm that the images actually got uploaded.
    self.assertEquals(self.bucket.datastore['path/to/white.png'],
                      image_tools.EncodePNG(self.white))
    self.assertEquals(self.bucket.datastore['path/to/black.png'],
                      image_tools.EncodePNG(self.black))
    self.assertEquals(self.bucket.datastore['path/to/red.png'],
                      image_tools.EncodePNG(self.red))

  def testDownloadImage(self):
    self.bucket.Reset()
    # Upload some images to the datastore.
    self.ispy_utils.UploadImage('path/to/white.png', self.white)
    self.ispy_utils.UploadImage('path/to/black.png', self.black)
    self.ispy_utils.UploadImage('path/to/red.png', self.red)
    # Check that the DownloadImage function gets the correct images.
    self.assertEquals(
        image_tools.EncodePNG(
            self.ispy_utils.DownloadImage('path/to/white.png')),
        image_tools.EncodePNG(self.white))
    self.assertEquals(
        image_tools.EncodePNG(
            self.ispy_utils.DownloadImage('path/to/black.png')),
        image_tools.EncodePNG(self.black))
    self.assertEquals(
        image_tools.EncodePNG(
            self.ispy_utils.DownloadImage('path/to/red.png')),
        image_tools.EncodePNG(self.red))
    # Check that the DownloadImage function throws an error for a
    #  nonexistant image.
    self.assertRaises(cloud_bucket.FileNotFoundError,
                      self.ispy_utils.DownloadImage,
                      'path/to/yellow.png')

  def testUpdateImage(self):
    self.bucket.Reset()
    # Upload some images to the datastore.
    self.ispy_utils.UploadImage('path/to/image.png', self.white)
    self.assertEquals(self.bucket.datastore['path/to/image.png'],
                      image_tools.EncodePNG(self.white))
    self.ispy_utils.UpdateImage('path/to/image.png', self.black)
    # Confirm that the image actually got updated.
    self.assertEquals(self.bucket.datastore['path/to/image.png'],
                      image_tools.EncodePNG(self.black))

  def testUploadTest(self):
    self.bucket.Reset()
    # Upload some tests to the datastore.
    self.ispy_utils.UploadTest('test', 'test1', [self.white, self.black])
    self.ispy_utils.UploadTest('test', 'test2', [self.black, self.black])
    # Confirm that the tests were successfully uploaded.
    self.assertEquals(self.bucket.datastore['tests/test/test1/expected.png'],
                      image_tools.EncodePNG(self.white))
    self.assertEquals(self.bucket.datastore['tests/test/test1/mask.png'],
                      image_tools.EncodePNG(self.white))
    self.assertEquals(self.bucket.datastore['tests/test/test2/expected.png'],
                      image_tools.EncodePNG(self.black))
    self.assertEquals(self.bucket.datastore['tests/test/test2/mask.png'],
                      image_tools.EncodePNG(self.black))

  def testRunTest(self):
    self.bucket.Reset()

    self.ispy_utils.UploadTest('test', 'test1', [self.red, self.red])
    self.ispy_utils.RunTest('test', 'test1', self.black)
    self.assertEquals(
        self.bucket.datastore['failures/test/test1/actual.png'],
        image_tools.EncodePNG(self.black))
    self.ispy_utils.RunTest('test', 'test1', self.red)
    self.assertTrue(
        self.bucket.datastore.has_key('failures/test/test1/actual.png'))

  def testGetTest(self):
    self.bucket.Reset()
    # Upload some tests to the datastore
    self.ispy_utils.UploadTest('test', 'test1', [self.white, self.black])
    self.ispy_utils.UploadTest('test', 'test2', [self.red, self.white])
    test1 = self.ispy_utils.GetTest('test', 'test1')
    test2 = self.ispy_utils.GetTest('test', 'test2')
    # Check that GetTest gets the appropriate tests.
    self.assertEquals(image_tools.EncodePNG(test1.expected),
                      image_tools.EncodePNG(self.white))
    self.assertEquals(image_tools.EncodePNG(test1.mask),
                      image_tools.EncodePNG(self.white))
    self.assertEquals(image_tools.EncodePNG(test2.expected),
                      image_tools.EncodePNG(self.red))
    self.assertEquals(image_tools.EncodePNG(test2.mask),
                      image_tools.EncodePNG(self.white))
    # Check that GetTest throws an error for a nonexistant test.
    self.assertRaises(
        cloud_bucket.FileNotFoundError, self.ispy_utils.GetTest, 'test',
        'test3')

  def testTestExists(self):
    self.bucket.Reset()
    self.ispy_utils.UploadTest('test', 'test1', [self.white, self.black])
    self.ispy_utils.UploadTest('test', 'test2', [self.white, self.black])
    self.assertTrue(self.ispy_utils.TestExists('test', 'test1'))
    self.assertTrue(self.ispy_utils.TestExists('test', 'test2'))
    self.assertFalse(self.ispy_utils.TestExists('test', 'test3'))

  def testFailureExists(self):
    self.bucket.Reset()
    self.ispy_utils.UploadTest('test', 'test1', [self.white, self.white])
    self.ispy_utils.RunTest('test', 'test1', self.black)
    self.ispy_utils.RunTest('test', 'test1', self.white)
    self.assertTrue(self.ispy_utils.FailureExists('test', 'test1'))
    self.assertFalse(self.ispy_utils.FailureExists('test', 'test2'))

  def testRemoveTest(self):
    self.bucket.Reset()
    self.ispy_utils.UploadTest('test', 'test1', [self.white, self.white])
    self.ispy_utils.UploadTest('test', 'test2', [self.white, self.white])
    self.assertTrue(self.ispy_utils.TestExists('test', 'test1'))
    self.assertTrue(self.ispy_utils.TestExists('test', 'test2'))
    self.ispy_utils.RemoveTest('test', 'test1')
    self.assertFalse(self.ispy_utils.TestExists('test', 'test1'))
    self.assertTrue(self.ispy_utils.TestExists('test', 'test2'))
    self.ispy_utils.RemoveTest('test', 'test2')
    self.assertFalse(self.ispy_utils.TestExists('test', 'test1'))
    self.assertFalse(self.ispy_utils.TestExists('test', 'test2'))

  def testRemoveFailure(self):
    self.bucket.Reset()
    self.ispy_utils.UploadTest('test', 'test1', [self.white, self.white])
    self.ispy_utils.UploadTest('test', 'test2', [self.white, self.white])
    self.ispy_utils.RunTest('test', 'test1', self.black)
    self.ispy_utils.RemoveFailure('test', 'test1')
    self.assertFalse(self.ispy_utils.FailureExists('test', 'test1'))
    self.assertTrue(self.ispy_utils.TestExists('test', 'test1'))
    self.assertFalse(self.ispy_utils.FailureExists('test', 'test2'))
    self.assertTrue(self.ispy_utils.TestExists('test', 'test2'))

  def testGetFailure(self):
    self.bucket.Reset()
    # Upload a result
    self.ispy_utils.UploadTest('test', 'test1', [self.red, self.red])
    self.ispy_utils.RunTest('test', 'test1', self.black)
    res = self.ispy_utils.GetFailure('test', 'test1')
    # Check that the function correctly got the result.
    self.assertEquals(image_tools.EncodePNG(res.expected),
                      image_tools.EncodePNG(self.red))
    self.assertEquals(image_tools.EncodePNG(res.diff),
                      image_tools.EncodePNG(self.masked))
    self.assertEquals(image_tools.EncodePNG(res.actual),
                      image_tools.EncodePNG(self.black))
    # Check that the function raises an error when given non-existant results.
    self.assertRaises(cloud_bucket.FileNotFoundError,
                      self.ispy_utils.GetFailure, 'test', 'test2')

  def testGetAllPaths(self):
    self.bucket.Reset()
    # Upload some tests.
    self.ispy_utils.UploadTest('test', 'test1', [self.white, self.black])
    self.ispy_utils.UploadTest('test', 'test2', [self.red, self.white])
    # Check that the function gets all urls matching the prefix.
    self.assertEquals(
        sets.Set(self.ispy_utils.GetAllPaths('tests/test/test')),
        sets.Set(['tests/test/test1/expected.png',
                  'tests/test/test1/mask.png',
                  'tests/test/test2/expected.png',
                  'tests/test/test2/mask.png']))
    self.assertEquals(
        sets.Set(self.ispy_utils.GetAllPaths('tests/test/test1')),
        sets.Set(['tests/test/test1/expected.png',
                  'tests/test/test1/mask.png']))
    self.assertEquals(
        sets.Set(self.ispy_utils.GetAllPaths('tests/test/test3')),
        sets.Set())


if __name__ == '__main__':
  unittest.main()
