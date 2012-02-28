// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Namespace object for file type utility functions.
 */
var FileType = {};

FileType.types = [
  // Images
  {type: 'image', name: 'IMAGE_FILE_TYPE', subtype: 'JPEG',
   pattern: /\.jpe?g$/i},
  {type: 'image', name: 'IMAGE_FILE_TYPE', subtype: 'BMP',
   pattern: /\.bmp$/i},
  {type: 'image', name: 'IMAGE_FILE_TYPE', subtype: 'GIF',
   pattern: /\.gif$/i},
  {type: 'image', name: 'IMAGE_FILE_TYPE', subtype: 'ICO',
   pattern: /\.ico$/i},
  {type: 'image', name: 'IMAGE_FILE_TYPE', subtype: 'PNG',
   pattern: /\.png$/i},
  {type: 'image', name: 'IMAGE_FILE_TYPE', subtype: 'WebP',
   pattern: /\.webp$/i},

  // Video
  {type: 'video', name: 'VIDEO_FILE_TYPE', subtype: '3GP',
   pattern: /\.3gp$/i},
  {type: 'video', name: 'VIDEO_FILE_TYPE', subtype: 'AVI',
   pattern: /\.avi$/i},
  {type: 'video', name: 'VIDEO_FILE_TYPE', subtype: 'QuickTime',
   pattern: /\.mov$/i},
  {type: 'video', name: 'VIDEO_FILE_TYPE', subtype: 'MPEG',
   pattern: /\.m(p4|4v|pg|peg|pg4|peg4)$/i},
  {type: 'video', name: 'VIDEO_FILE_TYPE', subtype: 'OGG',
   pattern: /\.og(m|v|x)$/i},
  {type: 'video', name: 'VIDEO_FILE_TYPE', subtype: 'WebM',
   pattern: /\.webm$/i},

  // Audio
  {type: 'audio', name: 'AUDIO_FILE_TYPE', subtype: 'FLAC',
   pattern: /\.flac$/i},
  {type: 'audio', name: 'AUDIO_FILE_TYPE', subtype: 'MP3',
   pattern: /\.mp3$/i},
  {type: 'audio', name: 'AUDIO_FILE_TYPE', subtype: 'MPEG',
   pattern: /\.m4a$/i},
  {type: 'audio', name: 'AUDIO_FILE_TYPE', subtype: 'OGG',
   pattern: /\.og(a|g)$/i},
  {type: 'audio', name: 'AUDIO_FILE_TYPE', subtype: 'WAV',
   pattern: /\.wav$/i},

  // Text
  {type: 'text', name: 'PLAIN_TEXT_FILE_TYPE', subtype: 'POD',
   pattern: /\.pod$/i},
  {type: 'text', name: 'PLAIN_TEXT_FILE_TYPE', subtype: 'RST',
   pattern: /\.rst$/i},
  {type: 'text', name: 'PLAIN_TEXT_FILE_TYPE', subtype: 'TXT',
   pattern: /\.txt$/i},
  {type: 'text', name: 'PLAIN_TEXT_FILE_TYPE', subtype: 'LOG',
   pattern: /\.log$/i},

  // Archive
  {type: 'archive', name: 'ZIP_ARCHIVE_FILE_TYPE', subtype:'ZIP',
   pattern: /\.zip$/i},
  {type: 'archive', name: 'RAR_ARCHIVE_FILE_TYPE', subtype:'RAR',
   pattern: /\.rar$/i},
  {type: 'archive', name: 'TAR_ARCHIVE_FILE_TYPE', subtype:'TAR',
   pattern: /\.tar$/i},
  {type: 'archive', name: 'TAR_BZIP2_ARCHIVE_FILE_TYPE', subtype:'TBZ2',
   pattern: /\.(tar.bz2|tbz|tbz2)$/i},
  {type: 'archive', name: 'TAR_GZIP_ARCHIVE_FILE_TYPE', subtype:'TGZ',
   pattern: /\.(tar.|t)gz$/i},

  // Others
  {type: 'text', icon: 'pdf', name: 'PDF_DOCUMENT_FILE_TYPE', subtype: 'PDF',
   pattern: /\.pdf$/i},
  {type: 'text', icon: 'html', name: 'HTML_DOCUMENT_FILE_TYPE', subtype: 'HTML',
   pattern: /\.html?$/i}
];

FileType.previewArt = {
  'audio': 'images/filetype_large_audio.png',
  'folder': 'images/filetype_large_folder.png',
  'unknown': 'images/filetype_large_generic.png',
  'image': 'images/filetype_large_image.png',
  'video': 'images/filetype_large_video.png'
};

/**
 * Get the file type object that matches a given url.
 *
 * @param {string} url
 * @return {Object} The matching file type object or an empty object.
 */
FileType.getType = function(url) {
  var types = FileType.types;
  for (var i = 0; i < types.length; i++) {
    if (types[i].pattern.test(url)) {
      return types[i];
    }
  }
  return {};
};

/**
 * Get the media type for a given url.
 *
 * @param {string} url
 * @return {string} The value of 'type' property from one of the elements in
 *   FileType.types or undefined.
 */
FileType.getMediaType = function(url) {
  return FileType.getType(url).type;
};

/**
 * Get the preview url for a given type.
 *
 * @param {string} type
 * @return {string}
 */
FileType.getPreviewArt = function(type) {
  return FileType.previewArt[type] || FileType.previewArt['unknown'];
};

FileType.MAX_PREVIEW_PIXEL_COUNT = 1 << 21; // 2 MPix
FileType.MAX_PREVIEW_FILE_SIZE = 1 << 20; // 1 Mb

/**
 * If an image file does not have an embedded thumbnail we might want to use
 * the image itself as a thumbnail. If the image is too large it hurts
 * the performance very much so we allow it only for moderately sized files.
 *
 * @param {Object} metadata
 * @param {number} opt_size The file size to be used if the metadata does not
 *   contain fileSize.
 * @return {boolean} Whether it is OK to use the image url for a preview.
 */
FileType.canUseImageUrlForPreview = function(metadata, opt_size) {
  var fileSize = metadata.fileSize || opt_size;
  return ((fileSize && fileSize <= FileType.MAX_PREVIEW_FILE_SIZE)  ||
      (metadata.width && metadata.height &&
      (metadata.width * metadata.height <= FileType.MAX_PREVIEW_PIXEL_COUNT)));
};
