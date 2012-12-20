// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview This file provides utility functions for position popups.
 */

cr.define('cr.ui', function() {

  /**
   * Type def for rects as returned by getBoundingClientRect.
   * @typedef { {left: number, top: number, width: number, height: number,
   *             right: number, bottom: number}}
   */
  var Rect;

  /** @const */
  var AnchorType = cr.ui.AnchorType;

  /**
   * @type {number}
   * @const
   */
  var BOOKMARK_BAR_HEIGHT = 48;

  /**
   * Helper function for positionPopupAroundElement and positionPopupAroundRect.
   * @param {!Rect} anchorRect The rect for the anchor.
   * @param {!HTMLElement} popupElement The element used for the popup.
   * @param {AnchorType} type The type of anchoring to do.
   * @param {boolean} invertLeftRight Whether to invert the right/left
   *     alignment.
   */
  function positionPopupAroundRect(anchorRect, popupElement, type,
                                   invertLeftRight) {
    var popupRect = popupElement.getBoundingClientRect();
    var availRect;
    var ownerDoc = popupElement.ownerDocument;
    var cs = ownerDoc.defaultView.getComputedStyle(popupElement);
    var docElement = ownerDoc.documentElement;

    if (cs.position == 'fixed') {
      // For 'fixed' positioned popups, the available rectangle should be based
      // on the viewport rather than the document.
      availRect = {
        height: docElement.clientHeight,
        width: docElement.clientWidth,
        top: 0,
        bottom: docElement.clientHeight,
        left: 0,
        right: docElement.clientWidth
      };
    } else {
      availRect = popupElement.offsetParent.getBoundingClientRect();
    }

    if (cs.direction == 'rtl')
      invertLeftRight = !invertLeftRight;

    // Flip BEFORE, AFTER based on alignment.
    if (invertLeftRight) {
      if (type == AnchorType.BEFORE)
        type = AnchorType.AFTER;
      else if (type == AnchorType.AFTER)
        type = AnchorType.BEFORE;
    }

    // Flip type based on available size
    switch (type) {
      case AnchorType.BELOW:
        // Do not flip when the type is below to avoid crbug.com/164113.
        break;
      case AnchorType.ABOVE:
        if (popupRect.height > anchorRect.top &&
            anchorRect.bottom + popupRect.height <= availRect.height) {
          type = AnchorType.BELOW;
        }
        break;
      case AnchorType.AFTER:
        if (anchorRect.right + popupRect.width > availRect.width &&
            popupRect.width <= anchorRect.left) {
          type = AnchorType.BEFORE;
        }
        break;
      case AnchorType.BEFORE:
        if (popupRect.width > anchorRect.left &&
            anchorRect.right + popupRect.width <= availRect.width) {
          type = AnchorType.AFTER;
        }
        break;
      default:
        assert(false, 'unknown type');
    }
    // flipping done

    var style = popupElement.style;
    // Reset all directions.
    style.left = style.right = style.top = style.bottom = 'auto';

    // Primary direction
    switch (type) {
      case AnchorType.BELOW:
        if (anchorRect.bottom + popupRect.height <= availRect.height -
            BOOKMARK_BAR_HEIGHT) {
          style.top = anchorRect.bottom + 'px';
        } else {
          style.bottom = BOOKMARK_BAR_HEIGHT + 'px';
        }
        break;
      case AnchorType.ABOVE:
        if (availRect.height - anchorRect.top >= 0)
          style.bottom = availRect.height - anchorRect.top + 'px';
        else
          style.top = '0';
        break;
      case AnchorType.AFTER:
        if (anchorRect.right + popupRect.width <= availRect.width)
          style.left = anchorRect.right + 'px';
        else
          style.right = '0';
        break;
      case AnchorType.BEFORE:
        if (availRect.width - anchorRect.left >= 0)
          style.right = availRect.width - anchorRect.left + 'px';
        else
          style.left = '0';
        break;
    }

    // Secondary direction
    switch (type) {
      case AnchorType.BELOW:
      case AnchorType.ABOVE:
        if (invertLeftRight) {
          // align right edges
          if (anchorRect.right - popupRect.width >= 0) {
            style.right = availRect.width - anchorRect.right + 'px';

          // align left edges
          } else if (anchorRect.left + popupRect.width <= availRect.width) {
            style.left = anchorRect.left + 'px';

          // not enough room on either side
          } else {
            style.right = '0';
          }
        } else {
          // align left edges
          if (anchorRect.left + popupRect.width <= availRect.width) {
            style.left = anchorRect.left + 'px';

          // align right edges
          } else if (anchorRect.right - popupRect.width >= 0) {
            style.right = availRect.width - anchorRect.right + 'px';

          // not enough room on either side
          } else {
            style.left = '0';
          }
        }
        break;

      case AnchorType.AFTER:
      case AnchorType.BEFORE:
        // align top edges
        if (anchorRect.top + popupRect.height <= availRect.height) {
          style.top = anchorRect.top + 'px';

        // align bottom edges
        } else if (anchorRect.bottom - popupRect.height >= 0) {
          style.bottom = availRect.height - anchorRect.bottom + 'px';

          // not enough room on either side
        } else {
          style.top = '0';
        }
        break;
    }
  }

  /**
   * Positions a popup element relative to an anchor element. The popup element
   * should have position set to absolute and it should be a child of the body
   * element.
   * @param {!HTMLElement} anchorElement The element that the popup is anchored
   *     to.
   * @param {!HTMLElement} popupElement The popup element we are positioning.
   * @param {AnchorType} type The type of anchoring we want.
   * @param {boolean} invertLeftRight Whether to invert the right/left
   *     alignment.
   */
  function positionPopupAroundElement(anchorElement, popupElement, type,
                                      invertLeftRight) {
    var anchorRect = anchorElement.getBoundingClientRect();
    positionPopupAroundRect(anchorRect, popupElement, type, invertLeftRight);
  }

  /**
   * Positions a popup around a point.
   * @param {number} x The client x position.
   * @param {number} y The client y position.
   * @param {!HTMLElement} popupElement The popup element we are positioning.
   */
  function positionPopupAtPoint(x, y, popupElement) {
    var rect = {
      left: x,
      top: y,
      width: 0,
      height: 0,
      right: x,
      bottom: y
    };
    positionPopupAroundRect(rect, popupElement, AnchorType.BELOW);
  }

  // Monkey patch popup positioning methods to avoid the popup being hidden
  // by the top overlay. This is a hacky solution, but temporarily necessary
  // until we get rid of the overlay madness.
  // TODO(pedrosimonetti): Remove this code when deprecating the NTP5 Apps.
  cr.ui.positionPopupAroundElement = positionPopupAroundElement;
  cr.ui.positionPopupAtPoint = positionPopupAtPoint;
});
