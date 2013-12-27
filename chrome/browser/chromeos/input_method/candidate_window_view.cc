// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "chrome/browser/chromeos/input_method/candidate_window_view.h"

#include <string>

#include "ash/shell.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chromeos/input_method/candidate_view.h"
#include "chrome/browser/chromeos/input_method/candidate_window_constants.h"
#include "chrome/browser/chromeos/input_method/hidable_area.h"
#include "chromeos/ime/candidate_window.h"
#include "ui/gfx/color_utils.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/widget/widget.h"

namespace chromeos {
namespace input_method {

namespace {
// VerticalCandidateLabel is used for rendering candidate text in
// the vertical candidate window.
class VerticalCandidateLabel : public views::Label {
 public:
  VerticalCandidateLabel() {}

 private:
  virtual ~VerticalCandidateLabel() {}

  // Returns the preferred size, but guarantees that the width has at
  // least kMinCandidateLabelWidth pixels.
  virtual gfx::Size GetPreferredSize() OVERRIDE {
    gfx::Size size = Label::GetPreferredSize();
    // Hack. +2 is needed to prevent labels from getting elided like
    // "abc..." in some cases. TODO(satorux): Figure out why it's
    // necessary.
    size.set_width(size.width() + 2);
    if (size.width() < kMinCandidateLabelWidth) {
      size.set_width(kMinCandidateLabelWidth);
    }
    if (size.width() > kMaxCandidateLabelWidth) {
      size.set_width(kMaxCandidateLabelWidth);
    }
    return size;
  }

  DISALLOW_COPY_AND_ASSIGN(VerticalCandidateLabel);
};

// Wraps the given view with some padding, and returns it.
views::View* WrapWithPadding(views::View* view, const gfx::Insets& insets) {
  views::View* wrapper = new views::View;
  // Use GridLayout to give some insets inside.
  views::GridLayout* layout = new views::GridLayout(wrapper);
  wrapper->SetLayoutManager(layout);  // |wrapper| owns |layout|.
  layout->SetInsets(insets);

  views::ColumnSet* column_set = layout->AddColumnSet(0);
  column_set->AddColumn(
      views::GridLayout::FILL, views::GridLayout::FILL,
      1, views::GridLayout::USE_PREF, 0, 0);
  layout->StartRow(0, 0);

  // Add the view contents.
  layout->AddView(view);  // |view| is owned by |wraper|, not |layout|.
  return wrapper;
}

// Creates shortcut text from the given index and the orientation.
base::string16 CreateShortcutText(size_t index,
                            const CandidateWindow& candidate_window) {
  if (index >= candidate_window.candidates().size())
    return base::string16();
  std::string shortcut_text = candidate_window.candidates()[index].label;
  if (!shortcut_text.empty() &&
      candidate_window.orientation() != CandidateWindow::VERTICAL)
    shortcut_text += '.';
  return base::UTF8ToUTF16(shortcut_text);
}

// Creates the shortcut label, and returns it (never returns NULL).
// The label text is not set in this function.
views::Label* CreateShortcutLabel(
    CandidateWindow::Orientation orientation, const ui::NativeTheme& theme) {
  // Create the shortcut label. The label will be owned by
  // |wrapped_shortcut_label|, hence it's deleted when
  // |wrapped_shortcut_label| is deleted.
  views::Label* shortcut_label = new views::Label;

  if (orientation == CandidateWindow::VERTICAL) {
    shortcut_label->SetFontList(
        shortcut_label->font_list().DeriveFontListWithSizeDeltaAndStyle(
            kFontSizeDelta, gfx::Font::BOLD));
  } else {
    shortcut_label->SetFontList(
        shortcut_label->font_list().DeriveFontListWithSizeDelta(
            kFontSizeDelta));
  }
  // TODO(satorux): Maybe we need to use language specific fonts for
  // candidate_label, like Chinese font for Chinese input method?
  shortcut_label->SetEnabledColor(theme.GetSystemColor(
      ui::NativeTheme::kColorId_LabelEnabledColor));
  shortcut_label->SetDisabledColor(theme.GetSystemColor(
      ui::NativeTheme::kColorId_LabelDisabledColor));

  return shortcut_label;
}

// Wraps the shortcut label, then decorates wrapped shortcut label
// and returns it (never returns NULL).
// The label text is not set in this function.
views::View* CreateWrappedShortcutLabel(
    views::Label* shortcut_label,
    CandidateWindow::Orientation orientation,
    const ui::NativeTheme& theme) {
  // Wrap it with padding.
  const gfx::Insets kVerticalShortcutLabelInsets(1, 6, 1, 6);
  const gfx::Insets kHorizontalShortcutLabelInsets(1, 3, 1, 0);
  const gfx::Insets insets =
      (orientation == CandidateWindow::VERTICAL ?
       kVerticalShortcutLabelInsets :
       kHorizontalShortcutLabelInsets);
  views::View* wrapped_shortcut_label =
      WrapWithPadding(shortcut_label, insets);

  // Add decoration based on the orientation.
  if (orientation == CandidateWindow::VERTICAL) {
    // Set the background color.
    SkColor blackish = color_utils::AlphaBlend(
        SK_ColorBLACK,
        theme.GetSystemColor(ui::NativeTheme::kColorId_WindowBackground),
        0x40);
    SkColor transparent_blakish = color_utils::AlphaBlend(
        SK_ColorTRANSPARENT, blackish, 0xE0);
    wrapped_shortcut_label->set_background(
        views::Background::CreateSolidBackground(transparent_blakish));
    shortcut_label->SetBackgroundColor(
        wrapped_shortcut_label->background()->get_color());
  }

  return wrapped_shortcut_label;
}

// Creates the candidate label, and returns it (never returns NULL).
// The label text is not set in this function.
views::Label* CreateCandidateLabel(
    CandidateWindow::Orientation orientation) {
  views::Label* candidate_label = NULL;

  // Create the candidate label. The label will be added to |this| as a
  // child view, hence it's deleted when |this| is deleted.
  if (orientation == CandidateWindow::VERTICAL) {
    candidate_label = new VerticalCandidateLabel;
  } else {
    candidate_label = new views::Label;
  }

  // Change the font size.
  candidate_label->SetFontList(
      candidate_label->font_list().DeriveFontListWithSizeDelta(kFontSizeDelta));
  candidate_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  return candidate_label;
}

// Creates the annotation label, and return it (never returns NULL).
// The label text is not set in this function.
views::Label* CreateAnnotationLabel(
    CandidateWindow::Orientation orientation, const ui::NativeTheme& theme) {
  // Create the annotation label.
  views::Label* annotation_label = new views::Label;

  // Change the font size and color.
  annotation_label->SetFontList(
      annotation_label->font_list().DeriveFontListWithSizeDelta(
          kFontSizeDelta));
  annotation_label->SetEnabledColor(theme.GetSystemColor(
      ui::NativeTheme::kColorId_LabelDisabledColor));
  annotation_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  return annotation_label;
}

// Computes shortcut column size.
gfx::Size ComputeShortcutColumnSize(
    const CandidateWindow& candidate_window,
    const ui::NativeTheme& theme) {
  int shortcut_column_width = 0;
  int shortcut_column_height = 0;
  // Create the shortcut label. The label will be owned by
  // |wrapped_shortcut_label|, hence it's deleted when
  // |wrapped_shortcut_label| is deleted.
  views::Label* shortcut_label = CreateShortcutLabel(
      candidate_window.orientation(), theme);
  scoped_ptr<views::View> wrapped_shortcut_label(
      CreateWrappedShortcutLabel(shortcut_label,
                                 candidate_window.orientation(),
                                 theme));

  // Compute the max width and height in shortcut labels.
  // We'll create temporary shortcut labels, and choose the largest width and
  // height.
  for (size_t i = 0; i < candidate_window.page_size(); ++i) {
    shortcut_label->SetText(CreateShortcutText(i, candidate_window));
    gfx::Size text_size = wrapped_shortcut_label->GetPreferredSize();
    shortcut_column_width = std::max(shortcut_column_width, text_size.width());
    shortcut_column_height = std::max(shortcut_column_height,
                                      text_size.height());
  }

  return gfx::Size(shortcut_column_width, shortcut_column_height);
}

// Computes the page index. For instance, if the page size is 9, and the
// cursor is pointing to 13th candidate, the page index will be 1 (2nd
// page, as the index is zero-origin). Returns -1 on error.
int ComputePageIndex(const CandidateWindow& candidate_window) {
  if (candidate_window.page_size() > 0)
    return candidate_window.cursor_position() / candidate_window.page_size();
  return -1;
}

// Computes candidate column size.
gfx::Size ComputeCandidateColumnSize(
    const CandidateWindow& candidate_window) {
  int candidate_column_width = 0;
  int candidate_column_height = 0;
  scoped_ptr<views::Label> candidate_label(
      CreateCandidateLabel(candidate_window.orientation()));

  // Compute the start index of |candidate_window_|.
  const int current_page_index = ComputePageIndex(candidate_window);
  if (current_page_index < 0)
    return gfx::Size(0, 0);
  const size_t start_from = current_page_index * candidate_window.page_size();

  // Compute the max width and height in candidate labels.
  // We'll create temporary candidate labels, and choose the largest width and
  // height.
  for (size_t i = 0;
       i + start_from < candidate_window.candidates().size();
       ++i) {
    const size_t index = start_from + i;

    candidate_label->SetText(
        base::UTF8ToUTF16(candidate_window.candidates()[index].value));
    gfx::Size text_size = candidate_label->GetPreferredSize();
    candidate_column_width = std::max(candidate_column_width,
                                      text_size.width());
    candidate_column_height = std::max(candidate_column_height,
                                       text_size.height());
  }

  return gfx::Size(candidate_column_width, candidate_column_height);
}

// Computes annotation column size.
gfx::Size ComputeAnnotationColumnSize(
    const CandidateWindow& candidate_window, const ui::NativeTheme& theme) {
  int annotation_column_width = 0;
  int annotation_column_height = 0;
  scoped_ptr<views::Label> annotation_label(
      CreateAnnotationLabel(candidate_window.orientation(), theme));

  // Compute the start index of |candidate_window_|.
  const int current_page_index = ComputePageIndex(candidate_window);
  if (current_page_index < 0)
    return gfx::Size(0, 0);
  const size_t start_from = current_page_index * candidate_window.page_size();

  // Compute max width and height in annotation labels.
  // We'll create temporary annotation labels, and choose the largest width and
  // height.
  for (size_t i = 0;
       i + start_from < candidate_window.candidates().size();
       ++i) {
    const size_t index = start_from + i;

    annotation_label->SetText(
        base::UTF8ToUTF16(candidate_window.candidates()[index].annotation));
    gfx::Size text_size = annotation_label->GetPreferredSize();
    annotation_column_width = std::max(annotation_column_width,
                                       text_size.width());
    annotation_column_height = std::max(annotation_column_height,
                                        text_size.height());
  }

  return gfx::Size(annotation_column_width, annotation_column_height);
}

}  // namespace

// InformationTextArea is a HidableArea having a single Label in it.
class InformationTextArea : public HidableArea {
 public:
  // Specify the alignment and initialize the control.
  InformationTextArea(gfx::HorizontalAlignment align, int minWidth)
      : minWidth_(minWidth) {
    label_ = new views::Label;
    label_->SetHorizontalAlignment(align);

    const gfx::Insets kInsets(2, 2, 2, 4);
    views::View* contents = WrapWithPadding(label_, kInsets);
    SetContents(contents);
    contents->set_border(views::Border::CreateSolidBorder(
        1,
        GetNativeTheme()->GetSystemColor(
            ui::NativeTheme::kColorId_MenuBorderColor)));
    contents->set_background(views::Background::CreateSolidBackground(
        color_utils::AlphaBlend(SK_ColorBLACK,
                                GetNativeTheme()->GetSystemColor(
                                    ui::NativeTheme::kColorId_WindowBackground),
                                0x10)));
    label_->SetBackgroundColor(contents->background()->get_color());
  }

  // Set the displayed text.
  void SetText(const std::string& utf8_text) {
    label_->SetText(base::UTF8ToUTF16(utf8_text));
  }

 protected:
  virtual gfx::Size GetPreferredSize() OVERRIDE {
    gfx::Size size = HidableArea::GetPreferredSize();
    // Hack. +2 is needed as the same reason as in VerticalCandidateLabel
    size.set_width(size.width() + 2);
    if (size.width() < minWidth_) {
      size.set_width(minWidth_);
    }
    return size;
  }

 private:
  views::Label* label_;
  int minWidth_;

  DISALLOW_COPY_AND_ASSIGN(InformationTextArea);
};

CandidateView::CandidateView(
    CandidateWindowView* parent_candidate_window,
    int index_in_page,
    CandidateWindow::Orientation orientation)
    : index_in_page_(index_in_page),
      orientation_(orientation),
      parent_candidate_window_(parent_candidate_window),
      shortcut_label_(NULL),
      candidate_label_(NULL),
      annotation_label_(NULL),
      infolist_icon_(NULL),
      infolist_icon_enabled_(false) {
}

void CandidateView::Init(int shortcut_column_width,
                         int candidate_column_width,
                         int annotation_column_width,
                         int column_height) {
  views::GridLayout* layout = new views::GridLayout(this);
  SetLayoutManager(layout);  // |this| owns |layout|.

  // Create Labels.
  const ui::NativeTheme& theme = *GetNativeTheme();
  shortcut_label_ = CreateShortcutLabel(orientation_, theme);
  views::View* wrapped_shortcut_label =
      CreateWrappedShortcutLabel(shortcut_label_, orientation_, theme);
  candidate_label_ = CreateCandidateLabel(orientation_);
  annotation_label_ = CreateAnnotationLabel(orientation_, theme);

  // Initialize the column set with three columns.
  views::ColumnSet* column_set = layout->AddColumnSet(0);

  // If orientation is vertical, each column width is fixed.
  // Otherwise the width is resizable.
  const views::GridLayout::SizeType column_type =
      orientation_ == CandidateWindow::VERTICAL ?
          views::GridLayout::FIXED : views::GridLayout::USE_PREF;

  const int padding_column_width =
      orientation_ == CandidateWindow::VERTICAL ? 4 : 6;

  // Set shortcut column type and width.
  column_set->AddColumn(views::GridLayout::FILL, views::GridLayout::FILL,
                        0, column_type, shortcut_column_width, 0);
  column_set->AddPaddingColumn(0, padding_column_width);

  // Set candidate column type and width.
  column_set->AddColumn(views::GridLayout::FILL, views::GridLayout::FILL,
                        1, views::GridLayout::USE_PREF, 0,
                        orientation_ == CandidateWindow::VERTICAL ?
                        candidate_column_width : 0);
  column_set->AddPaddingColumn(0, padding_column_width);

  // Set annotation column type and width.
  column_set->AddColumn(views::GridLayout::FILL, views::GridLayout::FILL,
                        0, column_type, annotation_column_width, 0);

  if (orientation_ == CandidateWindow::VERTICAL) {
    column_set->AddPaddingColumn(0, 1);
    column_set->AddColumn(views::GridLayout::FILL, views::GridLayout::FILL, 0,
                          views::GridLayout::FIXED, kInfolistIndicatorIconWidth,
                          0);
    column_set->AddPaddingColumn(0, 2);
  } else {
    column_set->AddPaddingColumn(0, padding_column_width);
  }

  // Add the shortcut label, the candidate label, and annotation label.
  layout->StartRow(0, 0);
  // |wrapped_shortcut_label|, |candidate_label_|, and |annotation_label_|
  // will be owned by |this|.
  layout->AddView(wrapped_shortcut_label,
                  1,  // Column span.
                  1,  // Row span.
                  views::GridLayout::FILL,  // Horizontal alignment.
                  views::GridLayout::FILL,  // Vertical alignment.
                  -1,  // Preferred width, not specified.
                  column_height);  // Preferred height.
  layout->AddView(candidate_label_,
                  1,  // Column span.
                  1,  // Row span.
                  views::GridLayout::FILL,  // Horizontal alignment.
                  views::GridLayout::FILL,  // Vertical alignment.
                  -1,  // Preferred width, not specified.
                  column_height);  // Preferred height.
  layout->AddView(annotation_label_,
                  1,  // Column span.
                  1,  // Row span.
                  views::GridLayout::FILL,  // Horizontal alignment.
                  views::GridLayout::FILL,  // Vertical alignemnt.
                  -1,  // Preferred width, not specified.
                  column_height);  // Preferred height.
  if (orientation_ == CandidateWindow::VERTICAL) {
    infolist_icon_ = new views::View;
    views::View* infolist_icon_wrapper = new views::View;
    views::GridLayout* infolist_icon_layout =
        new views::GridLayout(infolist_icon_wrapper);
    // |infolist_icon_layout| is owned by |infolist_icon_wrapper|.
    infolist_icon_wrapper->SetLayoutManager(infolist_icon_layout);
    infolist_icon_layout->AddColumnSet(0)->AddColumn(
        views::GridLayout::FILL, views::GridLayout::FILL,
        0, views::GridLayout::FIXED, kInfolistIndicatorIconWidth, 0);
    infolist_icon_layout->AddPaddingRow(0, kInfolistIndicatorIconPadding);
    infolist_icon_layout->StartRow(1.0, 0);  // infolist_icon_ is resizable.
    // |infolist_icon_| is owned by |infolist_icon_wrapper|.
    infolist_icon_layout->AddView(infolist_icon_);
    infolist_icon_layout->AddPaddingRow(0, kInfolistIndicatorIconPadding);
    // |infolist_icon_wrapper| is owned by |this|.
    layout->AddView(infolist_icon_wrapper);
  }
  UpdateLabelBackgroundColors();
}

void CandidateView::SetCandidateText(const base::string16& text) {
  candidate_label_->SetText(text);
}

void CandidateView::SetShortcutText(const base::string16& text) {
  shortcut_label_->SetText(text);
}

void CandidateView::SetAnnotationText(const base::string16& text) {
  annotation_label_->SetText(text);
}

void CandidateView::SetInfolistIcon(bool enable) {
  if (!infolist_icon_ || (infolist_icon_enabled_ == enable))
    return;
  infolist_icon_enabled_ = enable;
  infolist_icon_->set_background(
      enable ?
      views::Background::CreateSolidBackground(GetNativeTheme()->GetSystemColor(
          ui::NativeTheme::kColorId_FocusedBorderColor)) :
      NULL);
  UpdateLabelBackgroundColors();
  SchedulePaint();
}

void CandidateView::Select() {
  set_background(
      views::Background::CreateSolidBackground(GetNativeTheme()->GetSystemColor(
          ui::NativeTheme::kColorId_TextfieldSelectionBackgroundFocused)));
  set_border(views::Border::CreateSolidBorder(
      1, GetNativeTheme()->GetSystemColor(
          ui::NativeTheme::kColorId_FocusedBorderColor)));
  UpdateLabelBackgroundColors();
  // Need to call SchedulePaint() for background and border color changes.
  SchedulePaint();
}

void CandidateView::Unselect() {
  set_background(NULL);
  set_border(NULL);
  UpdateLabelBackgroundColors();
  SchedulePaint();  // See comments at Select().
}

void CandidateView::SetRowEnabled(bool enabled) {
  shortcut_label_->SetEnabled(enabled);
}

gfx::Point CandidateView::GetCandidateLabelPosition() const {
  return candidate_label_->GetMirroredPosition();
}

bool CandidateView::OnMousePressed(const ui::MouseEvent& event) {
  // TODO(kinaba): On Windows and MacOS, candidate windows typically commits a
  // candidate at OnMouseReleased event. We have chosen OnMousePressed here for
  // working around several obstacle rising from views implementation over GTK.
  // See: http://crosbug.com/11423#c11. Since we have moved from GTK to Aura,
  // the reasoning should have became obsolete. We might want to reconsider
  // implementing mouse-up selection.
  SelectCandidateAt(event.location());
  return false;
}

void CandidateView::OnGestureEvent(ui::GestureEvent* event) {
  if (event->type() == ui::ET_GESTURE_TAP) {
    SelectCandidateAt(event->location());
    event->SetHandled();
    return;
  }
  View::OnGestureEvent(event);
}

void CandidateView::SelectCandidateAt(const gfx::Point& location) {
  gfx::Point location_in_candidate_window = location;
  views::View::ConvertPointToTarget(this, parent_candidate_window_,
                                    &location_in_candidate_window);
  parent_candidate_window_->OnCandidatePressed(location_in_candidate_window);
  parent_candidate_window_->CommitCandidate();
}

void CandidateView::UpdateLabelBackgroundColors() {
  SkColor color = background() ?
      background()->get_color() :
      GetNativeTheme()->GetSystemColor(
          ui::NativeTheme::kColorId_WindowBackground);
  if (orientation_ != CandidateWindow::VERTICAL)
    shortcut_label_->SetBackgroundColor(color);
  candidate_label_->SetBackgroundColor(color);
  annotation_label_->SetBackgroundColor(color);
}

CandidateWindowView::CandidateWindowView(views::Widget* parent_frame)
    : selected_candidate_index_in_page_(-1),
      parent_frame_(parent_frame),
      preedit_area_(NULL),
      header_area_(NULL),
      candidate_area_(NULL),
      footer_area_(NULL),
      previous_shortcut_column_size_(0, 0),
      previous_candidate_column_size_(0, 0),
      previous_annotation_column_size_(0, 0),
      should_show_at_composition_head_(false),
      should_show_upper_side_(false),
      was_candidate_window_open_(false) {
}

CandidateWindowView::~CandidateWindowView() {
}

void CandidateWindowView::Init() {
  // Set the background and the border of the view.
  set_background(
      views::Background::CreateSolidBackground(GetNativeTheme()->GetSystemColor(
          ui::NativeTheme::kColorId_WindowBackground)));
  set_border(views::Border::CreateSolidBorder(
      1, GetNativeTheme()->GetSystemColor(
          ui::NativeTheme::kColorId_MenuBorderColor)));

  // Create areas.
  preedit_area_ = new InformationTextArea(gfx::ALIGN_LEFT,
                                          kMinPreeditAreaWidth);
  header_area_ = new InformationTextArea(gfx::ALIGN_LEFT, 0);
  candidate_area_ = new HidableArea;
  candidate_area_->SetContents(new views::View);
  footer_area_ = new InformationTextArea(gfx::ALIGN_RIGHT, 0);

  // Set the window layout of the view
  views::GridLayout* layout = new views::GridLayout(this);
  SetLayoutManager(layout);  // |this| owns |layout|.
  views::ColumnSet* column_set = layout->AddColumnSet(0);
  column_set->AddColumn(views::GridLayout::FILL, views::GridLayout::FILL,
                        0, views::GridLayout::USE_PREF, 0, 0);

  // Add the preedit area
  layout->StartRow(0, 0);
  layout->AddView(preedit_area_);  // |preedit_area_| is owned by |this|.

  // Add the header area.
  layout->StartRow(0, 0);
  layout->AddView(header_area_);  // |header_area_| is owned by |this|.

  // Add the candidate area.
  layout->StartRow(0, 0);
  layout->AddView(candidate_area_);  // |candidate_area_| is owned by |this|.

  // Add the footer area.
  layout->StartRow(0, 0);
  layout->AddView(footer_area_);  // |footer_area_| is owned by |this|.
}

void CandidateWindowView::HideAll() {
  parent_frame_->Hide();
  NotifyIfCandidateWindowOpenedOrClosed();
}

void CandidateWindowView::UpdateParentArea() {
  if (candidate_area_->IsShown() ||
      header_area_->IsShown() ||
      footer_area_->IsShown() ||
      preedit_area_->IsShown()) {
    ResizeAndMoveParentFrame();
    parent_frame_->Show();
  } else {
    parent_frame_->Hide();
  }
  NotifyIfCandidateWindowOpenedOrClosed();
}

void CandidateWindowView::HideLookupTable() {
  candidate_area_->Hide();
  UpdateParentArea();
}

void CandidateWindowView::HideAuxiliaryText() {
  header_area_->Hide();
  footer_area_->Hide();
  UpdateParentArea();
}

void CandidateWindowView::ShowAuxiliaryText() {
  // If candidate_area is not shown, shows auxiliary text at header_area.
  // We expect both header_area_ and footer_area_ contain same value.
  if (!candidate_area_->IsShown()) {
    header_area_->Show();
    footer_area_->Hide();
  } else {
    // If candidate_area is shown, shows auxiliary text with orientation.
    if (candidate_window_.orientation() == CandidateWindow::HORIZONTAL) {
      header_area_->Show();
      footer_area_->Hide();
    } else {
      footer_area_->Show();
      header_area_->Hide();
    }
  }
  UpdateParentArea();
}

void CandidateWindowView::UpdateAuxiliaryText(const std::string& utf8_text) {
  header_area_->SetText(utf8_text);
  footer_area_->SetText(utf8_text);
  ShowAuxiliaryText();
}

void CandidateWindowView::HidePreeditText() {
  preedit_area_->Hide();
  UpdateParentArea();
}

void CandidateWindowView::ShowPreeditText() {
  preedit_area_->Show();
  UpdateParentArea();
}

void CandidateWindowView::UpdatePreeditText(const std::string& utf8_text) {
  preedit_area_->SetText(utf8_text);
}

void CandidateWindowView::ShowLookupTable() {
  if (!candidate_area_->IsShown())
    should_show_upper_side_ = false;
  candidate_area_->Show();
  UpdateParentArea();
}

void CandidateWindowView::NotifyIfCandidateWindowOpenedOrClosed() {
  bool is_open = IsCandidateWindowOpen();
  if (!was_candidate_window_open_ && is_open) {
    FOR_EACH_OBSERVER(Observer, observers_, OnCandidateWindowOpened());
  } else if (was_candidate_window_open_ && !is_open) {
    FOR_EACH_OBSERVER(Observer, observers_, OnCandidateWindowClosed());
  }
  was_candidate_window_open_ = is_open;
}

bool CandidateWindowView::ShouldUpdateCandidateViews(
    const CandidateWindow& old_candidate_window,
    const CandidateWindow& new_candidate_window) {
  return !old_candidate_window.IsEqual(new_candidate_window);
}

void CandidateWindowView::UpdateCandidates(
    const CandidateWindow& new_candidate_window) {
  const bool should_update = ShouldUpdateCandidateViews(candidate_window_,
                                                        new_candidate_window);
  // Updating the candidate views is expensive. We'll skip this if possible.
  if (should_update) {
    // Initialize candidate views if necessary.
    MaybeInitializeCandidateViews(new_candidate_window);

    should_show_at_composition_head_
        = new_candidate_window.show_window_at_composition();
    // Compute the index of the current page.
    const int current_page_index = ComputePageIndex(new_candidate_window);
    if (current_page_index < 0) {
      return;
    }

    // Update the candidates in the current page.
    const size_t start_from =
        current_page_index * new_candidate_window.page_size();

    // In some cases, engines send empty shortcut labels. For instance,
    // ibus-mozc sends empty labels when they show suggestions. In this
    // case, we should not show shortcut labels.
    bool no_shortcut_mode = true;
    for (size_t i = 0; i < new_candidate_window.candidates().size(); ++i) {
      if (!new_candidate_window.candidates()[i].label.empty()) {
        no_shortcut_mode = false;
        break;
      }
    }

    for (size_t i = 0; i < candidate_views_.size(); ++i) {
      const size_t index_in_page = i;
      const size_t candidate_index = start_from + index_in_page;
      CandidateView* candidate_view = candidate_views_[index_in_page];
      // Set the shortcut text.
      if (no_shortcut_mode) {
        candidate_view->SetShortcutText(base::string16());
      } else {
        // At this moment, we don't use labels sent from engines for UX
        // reasons. First, we want to show shortcut labels in empty rows
        // (ex. show 6, 7, 8, ... in empty rows when the number of
        // candidates is 5). Second, we want to add a period after each
        // shortcut label when the candidate window is horizontal.
        candidate_view->SetShortcutText(
            CreateShortcutText(i, new_candidate_window));
      }
      // Set the candidate text.
       if (candidate_index < new_candidate_window.candidates().size()) {
         const CandidateWindow::Entry& entry =
             new_candidate_window.candidates()[candidate_index];
         candidate_view->SetCandidateText(base::UTF8ToUTF16(entry.value));
         candidate_view->SetAnnotationText(base::UTF8ToUTF16(entry.annotation));
         candidate_view->SetRowEnabled(true);
         candidate_view->SetInfolistIcon(!entry.description_title.empty());
      } else {
        // Disable the empty row.
        candidate_view->SetCandidateText(base::string16());
        candidate_view->SetAnnotationText(base::string16());
        candidate_view->SetRowEnabled(false);
        candidate_view->SetInfolistIcon(false);
      }
    }
  }
  // Update the current candidate window. We'll use candidate_window_ from here.
  // Note that SelectCandidateAt() uses candidate_window_.
  candidate_window_.CopyFrom(new_candidate_window);

  // Select the current candidate in the page.
  if (candidate_window_.is_cursor_visible()) {
    if (candidate_window_.page_size()) {
      const int current_candidate_in_page =
          candidate_window_.cursor_position() % candidate_window_.page_size();
      SelectCandidateAt(current_candidate_in_page);
    }
  } else {
    // Unselect the currently selected candidate.
    if (0 <= selected_candidate_index_in_page_ &&
        static_cast<size_t>(selected_candidate_index_in_page_) <
        candidate_views_.size()) {
      candidate_views_[selected_candidate_index_in_page_]->Unselect();
      selected_candidate_index_in_page_ = -1;
    }
  }
}

void CandidateWindowView::MaybeInitializeCandidateViews(
    const CandidateWindow& candidate_window) {
  const CandidateWindow::Orientation orientation =
      candidate_window.orientation();
  const int page_size = candidate_window.page_size();
  views::View* candidate_area_contents = candidate_area_->contents();

  // Current column width.
  gfx::Size shortcut_column_size(0, 0);
  gfx::Size candidate_column_size(0,0);
  gfx::Size annotation_column_size(0, 0);

  // If orientation is horizontal, don't need to compute width,
  // because each label is left aligned.
  if (orientation == CandidateWindow::VERTICAL) {
    const ui::NativeTheme& theme = *GetNativeTheme();
    shortcut_column_size = ComputeShortcutColumnSize(candidate_window, theme);
    candidate_column_size = ComputeCandidateColumnSize(candidate_window);
    annotation_column_size = ComputeAnnotationColumnSize(candidate_window,
                                                         theme);
  }

  // If the requested number of views matches the number of current views, and
  // previous and current column width are same, just reuse these.
  //
  // Note that the early exit logic is not only useful for improving
  // performance, but also necessary for the horizontal candidate window
  // to be redrawn properly. If we get rid of the logic, the horizontal
  // candidate window won't get redrawn properly for some reason when
  // there is no size change. You can test this by removing "return" here
  // and type "ni" with Pinyin input method.
  if (static_cast<int>(candidate_views_.size()) == page_size &&
      candidate_window_.orientation() == orientation &&
      previous_shortcut_column_size_ == shortcut_column_size &&
      previous_candidate_column_size_ == candidate_column_size &&
      previous_annotation_column_size_ == annotation_column_size) {
    return;
  }

  // Update the previous column widths.
  previous_shortcut_column_size_ = shortcut_column_size;
  previous_candidate_column_size_ = candidate_column_size;
  previous_annotation_column_size_ = annotation_column_size;

  // Clear the existing candidate_views if any.
  for (size_t i = 0; i < candidate_views_.size(); ++i) {
    candidate_area_contents->RemoveChildView(candidate_views_[i]);
    // Delete the view after getting out the current message loop iteration.
    base::MessageLoop::current()->DeleteSoon(FROM_HERE, candidate_views_[i]);
  }
  candidate_views_.clear();
  selected_candidate_index_in_page_ = -1;  // Invalidates the index.

  views::GridLayout* layout = new views::GridLayout(candidate_area_contents);
  // |candidate_area_contents| owns |layout|.
  candidate_area_contents->SetLayoutManager(layout);
  // Initialize the column set.
  views::ColumnSet* column_set = layout->AddColumnSet(0);
  if (orientation == CandidateWindow::VERTICAL) {
    column_set->AddColumn(views::GridLayout::FILL,
                          views::GridLayout::FILL,
                          1, views::GridLayout::USE_PREF, 0, 0);
  } else {
    for (int i = 0; i < page_size; ++i) {
      column_set->AddColumn(views::GridLayout::FILL,
                            views::GridLayout::FILL,
                            0, views::GridLayout::USE_PREF, 0, 0);
    }
  }

  // Set insets so the border of the selected candidate is drawn inside of
  // the border of the main candidate window, but we don't have the inset
  // at the top and the bottom as we have the borders of the header and
  // footer areas.
  const gfx::Insets kCandidateAreaInsets(0, 1, 0, 1);
  layout->SetInsets(kCandidateAreaInsets.top(),
                    kCandidateAreaInsets.left(),
                    kCandidateAreaInsets.bottom(),
                    kCandidateAreaInsets.right());

  // Use maximum height for all rows in candidate area.
  const int kColumnHeight = std::max(shortcut_column_size.height(),
                                     std::max(candidate_column_size.height(),
                                              annotation_column_size.height()));

  // Add views to the candidate area.
  if (orientation == CandidateWindow::HORIZONTAL) {
    layout->StartRow(0, 0);
  }

  for (int i = 0; i < page_size; ++i) {
    CandidateView* candidate_row = new CandidateView(this, i, orientation);
    candidate_row->Init(shortcut_column_size.width(),
                        candidate_column_size.width(),
                        annotation_column_size.width(),
                        kColumnHeight);
    candidate_views_.push_back(candidate_row);
    if (orientation == CandidateWindow::VERTICAL) {
      layout->StartRow(0, 0);
    }
    // |candidate_row| will be owned by |candidate_area_contents|.
    layout->AddView(candidate_row,
                    1,  // Column span.
                    1,  // Row span.
                    // Horizontal alignment.
                    orientation == CandidateWindow::VERTICAL ?
                    views::GridLayout::FILL : views::GridLayout::CENTER,
                    views::GridLayout::CENTER,  // Vertical alignment.
                    -1,  // Preferred width, not specified.
                    kColumnHeight);  // Preferred height.
  }

  // Compute views size in |layout|.
  // If we don't call this function, GetHorizontalOffset() often
  // returns invalid value (returns 0), then candidate window
  // moves right from the correct position in ResizeAndMoveParentFrame().
  // TODO(nhiroki): Figure out why it returns invalid value.
  // It seems that the x-position of the candidate labels is not set.
  layout->Layout(candidate_area_contents);
}

bool CandidateWindowView::IsCandidateWindowOpen() const {
  return !should_show_at_composition_head_ &&
      candidate_area_->visible() && candidate_area_->IsShown();
}

void CandidateWindowView::SelectCandidateAt(int index_in_page) {
  const int current_page_index = ComputePageIndex(candidate_window_);
  if (current_page_index < 0) {
    return;
  }

  const int cursor_absolute_index =
      candidate_window_.page_size() * current_page_index + index_in_page;
  // Ignore click on out of range views.
  if (cursor_absolute_index < 0 ||
      candidate_window_.candidates().size() <=
      static_cast<size_t>(cursor_absolute_index)) {
    return;
  }

  // Unselect the currently selected candidate.
  if (0 <= selected_candidate_index_in_page_ &&
      static_cast<size_t>(selected_candidate_index_in_page_) <
      candidate_views_.size()) {
    candidate_views_[selected_candidate_index_in_page_]->Unselect();
  }
  // Remember the currently selected candidate index in the current page.
  selected_candidate_index_in_page_ = index_in_page;

  // Select the candidate specified by index_in_page.
  candidate_views_[index_in_page]->Select();

  // Update the cursor indexes in the model.
  candidate_window_.set_cursor_position(cursor_absolute_index);
}

void CandidateWindowView::OnCandidatePressed(
    const gfx::Point& location) {
  for (size_t i = 0; i < candidate_views_.size(); ++i) {
    gfx::Point converted_location = location;
    views::View::ConvertPointToTarget(this, candidate_views_[i],
                                      &converted_location);
    if (candidate_views_[i]->HitTestPoint(converted_location)) {
      SelectCandidateAt(i);
      break;
    }
  }
}

void CandidateWindowView::CommitCandidate() {
  if (!(0 <= selected_candidate_index_in_page_ &&
        static_cast<size_t>(selected_candidate_index_in_page_) <
        candidate_views_.size())) {
    return;  // Out of range, do nothing.
  }

  FOR_EACH_OBSERVER(Observer, observers_,
                    OnCandidateCommitted(selected_candidate_index_in_page_));
}

void CandidateWindowView::ResizeAndMoveParentFrame() {
  // If rendering operation comes from mozc-engine, uses mozc specific bounds,
  // otherwise candidate window is shown under the cursor.
  const int x = should_show_at_composition_head_?
      composition_head_bounds_.x() : cursor_bounds_.x();
  // To avoid candidate-window overlapping, uses maximum y-position of mozc
  // specific bounds and cursor bounds, because mozc-engine does not
  // consider about multi-line composition.
  const int y = should_show_at_composition_head_?
      std::max(composition_head_bounds_.y(), cursor_bounds_.y()) :
      cursor_bounds_.y();
  const int height = cursor_bounds_.height();
  const int horizontal_offset = GetHorizontalOffset();

  gfx::Rect old_bounds = parent_frame_->GetClientAreaBoundsInScreen();
  gfx::Rect screen_bounds = ash::Shell::GetScreen()->GetDisplayMatching(
      cursor_bounds_).work_area();
  // The size.
  gfx::Rect frame_bounds = old_bounds;
  frame_bounds.set_size(GetPreferredSize());

  // The default position.
  frame_bounds.set_x(x + horizontal_offset);
  frame_bounds.set_y(y + height);

  // Handle overflow at the left and the top.
  frame_bounds.set_x(std::max(frame_bounds.x(), screen_bounds.x()));
  frame_bounds.set_y(std::max(frame_bounds.y(), screen_bounds.y()));

  // Handle overflow at the right.
  const int right_overflow = frame_bounds.right() - screen_bounds.right();
  if (right_overflow > 0) {
    frame_bounds.set_x(frame_bounds.x() - right_overflow);
  }

  // Handle overflow at the bottom.
  const int bottom_overflow = frame_bounds.bottom() - screen_bounds.bottom();

  // To avoid flickering window position, the candidate window should be shown
  // on upper side of composition string if it was shown there.
  if (should_show_upper_side_ || bottom_overflow > 0) {
    frame_bounds.set_y(frame_bounds.y() - height - frame_bounds.height());
    should_show_upper_side_ = true;
  }

  // TODO(nona): check top_overflow here.

  // Move the window per the cursor bounds.
  // SetBounds() is not cheap. Only call this when it is really changed.
  if (frame_bounds != old_bounds)
    parent_frame_->SetBounds(frame_bounds);
}

int CandidateWindowView::GetHorizontalOffset() {
  // Compute the horizontal offset if the candidate window is vertical.
  if (!candidate_views_.empty() &&
      candidate_window_.orientation() == CandidateWindow::VERTICAL) {
    return - candidate_views_[0]->GetCandidateLabelPosition().x();
  }
  return 0;
}

void CandidateWindowView::VisibilityChanged(View* starting_from,
                                            bool is_visible) {
  if (is_visible) {
    // If the visibility of candidate window is changed,
    // we should move the frame to the right position.
    ResizeAndMoveParentFrame();
  }
}

void CandidateWindowView::OnBoundsChanged(const gfx::Rect& previous_bounds) {
  // If the bounds(size) of candidate window is changed,
  // we should move the frame to the right position.
  View::OnBoundsChanged(previous_bounds);
  ResizeAndMoveParentFrame();
}

}  // namespace input_method
}  // namespace chromeos
