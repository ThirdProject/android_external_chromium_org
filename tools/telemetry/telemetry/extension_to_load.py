# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import os

from telemetry import crx_id

class ExtensionPathNonExistentException(Exception):
  pass

class ExtensionToLoad(object):
  def __init__(self, path, is_component=False):
    if not os.path.isdir(path):
      raise ExtensionPathNonExistentException(
          'Extension path not a directory %s' % path)
    self._path = path
    self._is_component = is_component

  @property
  def extension_id(self):
    """Unique extension id of this extension."""
    return crx_id.GetCRXAppID(os.path.abspath(self._path))

  @property
  def path(self):
    """Path to extension directory."""
    return self._path

  @property
  def is_component(self):
    """Whether this extension should be loaded as a component extension."""
    return self._is_component
