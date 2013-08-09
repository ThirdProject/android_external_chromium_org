// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file provides utility functions for "file tasks".
//
// WHAT ARE FILE TASKS?
//
// File tasks are representatiosn of actions that can be performed over the
// currently selected files from Files.app. A task can be either of:
//
// 1) Chrome extension or app, registered via "file_handlers" or
// "file_browser_handlers" in manifest.json (ex. Text.app). This information
// comes from FileBrowserHandler::GetHandlers()
//
// See also:
// https://developer.chrome.com/extensions/manifest.html#file_handlers
// https://developer.chrome.com/extensions/fileBrowserHandler.html
//
// 2) Built-in handlers provided from Files.app. Files.app provides lots of
// file_browser_handlers, such as "play", "watch", "mount-archive".  These
// built-in handlers are often handled in special manners inside Files.app.
// This information also comes from FileBrowserHandler::GetHandlers().
//
// See also:
// chrome/browser/resources/file_manager/manifest.json
//
// 3) Drive app, which is a hosted app (i.e. just web site), that can work
// with Drive (ex. Pixlr Editor). This information comes from
// drive::DriveAppRegistry.
//
// See also:
// https://chrome.google.com/webstore/category/collection/drive_apps
//
// For example, if the user is now selecting a JPEG file, Files.app will
// receive file tasks represented as a JSON object via
// chrome.fileBrowserPrivate.getFileTasks() API, which look like:
//
// [
//   {
//     "driveApp": true,
//     "iconUrl": "<app_icon_url>",
//     "isDefault": false,
//     "taskId": "<drive_app_id>|drive|open-with",
//     "title": "Drive App Name (ex. Pixlr Editor)"
//   },
//   {
//     "driveApp": false,
//     "iconUrl": "chrome://extension-icon/hhaomjibdihmijegdhdafkllkbggdgoj/16/1",
//     "isDefault": true,
//     "taskId": "hhaomjibdihmijegdhdafkllkbggdgoj|file|gallery",
//     "title": "__MSG_OPEN_ACTION__"
//   }
// ]
//
// The first file task is a Drive app. The second file task is a built-in
// handler from Files.app.
//
// WHAT ARE TASK IDS?
//
// You may have noticed that "taskId" fields in the above example look
// awakard. Apparently "taskId" encodes three types of information delimited
// by "|". This is a weird format for something called as an ID.
//
// 1) Why are the three types information encoded in this way?
//
// It's just a historical reason. The reason is that a simple string can be
// easily stored in user's preferences. We should stop doing this, by storing
// this information in chrome.storage instead. crbug.com/267359.
//
// 2) OK, then what are the three types of information encoded here?
//
// The task ID encodes the folloing structure:
//
//     <app-id>|<task-type>|<task-action-id>
//
// <app-id> is either of Chrome Extension/App ID or Drive App ID. For some
// reason, Chrome Extension/App IDs and Drive App IDs look differently. As of
// writing, the fomer looks like "hhaomjibdihmijegdhdafkllkbggdgoj"
// (Files.app) and the latter looks like "419782477519" (Pixlr Editor).
//
// <task-type> is either of
// - "file" - File browser handler - app/extension declaring
//            "file_browser_handlers" in manifest.
// - "app" - File handler - app declaring "file_handlers" in manifest.json.
// - "drive" - Drive App
//
// <task-action-id> is an ID string used for identifying actions provided
// from a single Chrome Extension/App. In other words, a single
// Chrome/Extension can provide multiple file handlers hence each of them
// needs to have a unique action ID.  For Drive apps, <task-action-id> is
// always "open-with".
//
// HOW TASKS ARE EXECUTED?
//
// chrome.fileBrowserPrivate.viewFiles() is used to open a file in a browser,
// without any handler. Browser will take care of handling the file (ex. PDF).
//
// chrome.fileBrowserPrivate.executeTasks() is used to open a file with a
// handler (Chrome Extension/App or Drive App).
//
// Some built-in handlers such as "play" and "watch" are handled internally
// in Files.app. "mount-archive" is handled very differently. The task
// execution business should be simplified: crbug.com/267313
//
// See also:
// chrome/browser/resources/file_manager/js/file_tasks.js
//

#ifndef CHROME_BROWSER_CHROMEOS_EXTENSIONS_FILE_MANAGER_FILE_TASKS_H_
#define CHROME_BROWSER_CHROMEOS_EXTENSIONS_FILE_MANAGER_FILE_TASKS_H_

#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/platform_file.h"
#include "chrome/common/extensions/extension.h"

class Browser;
class FileBrowserHandler;
class GURL;
class Profile;

namespace fileapi {
class FileSystemURL;
}

namespace file_manager {
namespace file_tasks {

// Tasks are stored as a vector in order of priorities.
typedef std::vector<const FileBrowserHandler*> FileBrowserHandlerList;

// Task types encoded in task IDs. See also the comment at the beginning of
// the file about <task-type>.
extern const char kFileBrowserHandlerTaskType[];
extern const char kFileHandlerTaskType[];
extern const char kDriveTaskType[];

// Returns true if the given file browser handler should be used as a
// fallback. Such handlers are Files.app's internal handlers as well as quick
// office extensions.
bool IsFallbackFileBrowserHandler(const FileBrowserHandler* handler);

// Update the default file handler for the given sets of suffixes and MIME
// types.
void UpdateDefaultTask(Profile* profile,
                       const std::string& task_id,
                       const std::set<std::string>& suffixes,
                       const std::set<std::string>& mime_types);

// Returns the task ID of the default task for the given |mime_type|/|suffix|
// combination. If it finds a MIME type match, then it prefers that over a
// suffix match. If it a default can't be found, then it returns the empty
// string.
std::string GetDefaultTaskIdFromPrefs(Profile* profile,
                                      const std::string& mime_type,
                                      const std::string& suffix);

// Generates task id for the action specified by the extension. The
// |task_type| must be one of kFileBrowserHandlerTaskType, kDriveTaskType or
// kFileHandlerTaskType.
std::string MakeTaskID(const std::string& extension_id,
                       const std::string& task_type,
                       const std::string& action_id);

// Extracts action, type and extension id bound to the file task ID. Either
// |target_extension_id| or |action_id| are allowed to be NULL if caller isn't
// interested in those values.  Returns false on failure to parse.
//
// See also the comment at the beginning of the file for details for how
// "task_id" looks like.
bool CrackTaskID(const std::string& task_id,
                 std::string* target_extension_id,
                 std::string* task_type,
                 std::string* action_id);

// Finds file browser handlers set as default from |common_tasks| for
// |file_list|. If no handlers are set as default, choose the the firstly
// found fallback handler as default.
FileBrowserHandlerList FindDefaultFileBrowserHandlers(
    Profile* profile,
    const std::vector<base::FilePath>& files_list,
    const FileBrowserHandlerList& common_tasks);

// Returns the list of file browser handlers that can open all files in
// |file_list|.
FileBrowserHandlerList FindCommonFileBrowserHandlers(
    Profile* profile,
    const std::vector<GURL>& files_list);

// Finds a file browser handler for a file whose URL is |url| and whose path
// is |path|. Returns the default handler if one is defined (The default
// handler is the one that is assigned to the file manager task button by
// default). If the default handler is not found, tries to match the url with
// one of the file browser handlers.
const FileBrowserHandler* FindFileBrowserHandlerForURLAndPath(
    Profile* profile,
    const GURL& url,
    const base::FilePath& path);

// The callback is used for ExecuteFileTask(). Will be called with true if
// the file task execution is successful, or false if unsuccessful.
typedef base::Callback<void(bool success)> FileTaskFinishedCallback;

// Executes file handler task for each element of |file_urls|.
// Returns |false| if the execution cannot be initiated. Otherwise returns
// |true| and then eventually calls |done| when all the files have been handled.
// |done| can be a null callback.
//
// Parameters:
// profile    - The profile used for making this function call.
// source_url - The source URL which originates this function call.
// tab_id     - The ID of the tab which originates this function call.
//              This can be 0 if no tab is associated.
// app_id     - See the comment at the beginning of the file for <app-id>.
// task_type  - See the comment at the beginning of the file for <task-type>.
// action_id  - See the comment at the beginning of the file for <action-id>.
// file_urls  - URLs of the target files.
// done       - The callback which will be called on completion.
//              The callback won't be called if the function returns false.
bool ExecuteFileTask(Profile* profile,
                     const GURL& source_url,
                     const std::string& file_browser_id,
                     int32 tab_id,
                     const std::string& app_id,
                     const std::string& task_type,
                     const std::string& action_id,
                     const std::vector<fileapi::FileSystemURL>& file_urls,
                     const FileTaskFinishedCallback& done);

}  // namespace file_tasks
}  // namespace file_manager

#endif  // CHROME_BROWSER_CHROMEOS_EXTENSIONS_FILE_MANAGER_FILE_TASKS_H_
