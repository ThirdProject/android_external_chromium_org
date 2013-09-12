// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/drive/drive_api_util.h"

#include <string>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/drive/drive_switches.h"
#include "chrome/browser/google_apis/drive_api_parser.h"
#include "chrome/browser/google_apis/gdata_wapi_parser.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/escape.h"
#include "third_party/re2/re2/re2.h"
#include "url/gurl.h"

namespace drive {
namespace util {

bool IsDriveV2ApiEnabled() {
  const CommandLine* command_line = CommandLine::ForCurrentProcess();

  // Enable Drive API v2 by default.
  if (!command_line->HasSwitch(switches::kEnableDriveV2Api))
    return true;

  std::string value =
      command_line->GetSwitchValueASCII(switches::kEnableDriveV2Api);
  StringToLowerASCII(&value);
  // The value must be "" or "true" for true, or "false" for false.
  DCHECK(value.empty() || value == "true" || value == "false");
  return value != "false";
}

std::string EscapeQueryStringValue(const std::string& str) {
  std::string result;
  result.reserve(str.size());
  for (size_t i = 0; i < str.size(); ++i) {
    if (str[i] == '\\' || str[i] == '\'') {
      result.push_back('\\');
    }
    result.push_back(str[i]);
  }
  return result;
}

std::string TranslateQuery(const std::string& original_query) {
  // In order to handle non-ascii white spaces correctly, convert to UTF16.
  base::string16 query = UTF8ToUTF16(original_query);
  const base::string16 kDelimiter(
      kWhitespaceUTF16 + base::string16(1, static_cast<char16>('"')));

  std::string result;
  for (size_t index = query.find_first_not_of(kWhitespaceUTF16);
       index != base::string16::npos;
       index = query.find_first_not_of(kWhitespaceUTF16, index)) {
    bool is_exclusion = (query[index] == '-');
    if (is_exclusion)
      ++index;
    if (index == query.length()) {
      // Here, the token is '-' and it should be ignored.
      continue;
    }

    size_t begin_token = index;
    base::string16 token;
    if (query[begin_token] == '"') {
      // Quoted query.
      ++begin_token;
      size_t end_token = query.find('"', begin_token);
      if (end_token == base::string16::npos) {
        // This is kind of syntax error, since quoted string isn't finished.
        // However, the query is built by user manually, so here we treat
        // whole remaining string as a token as a fallback, by appending
        // a missing double-quote character.
        end_token = query.length();
        query.push_back('"');
      }

      token = query.substr(begin_token, end_token - begin_token);
      index = end_token + 1;  // Consume last '"', too.
    } else {
      size_t end_token = query.find_first_of(kDelimiter, begin_token);
      if (end_token == base::string16::npos) {
        end_token = query.length();
      }

      token = query.substr(begin_token, end_token - begin_token);
      index = end_token;
    }

    if (token.empty()) {
      // Just ignore an empty token.
      continue;
    }

    if (!result.empty()) {
      // If there are two or more tokens, need to connect with "and".
      result.append(" and ");
    }

    // The meaning of "fullText" should include title, description and content.
    base::StringAppendF(
        &result,
        "%sfullText contains \'%s\'",
        is_exclusion ? "not " : "",
        EscapeQueryStringValue(UTF16ToUTF8(token)).c_str());
  }

  return result;
}

std::string ExtractResourceIdFromUrl(const GURL& url) {
  return net::UnescapeURLComponent(url.ExtractFileName(),
                                   net::UnescapeRule::URL_SPECIAL_CHARS);
}

std::string CanonicalizeResourceId(const std::string& resource_id) {
  // If resource ID is in the old WAPI format starting with a prefix like
  // "document:", strip it and return the remaining part.
  std::string stripped_resource_id;
  if (RE2::FullMatch(resource_id, "^[a-z-]+(?::|%3A)([\\w-]+)$",
                     &stripped_resource_id))
    return stripped_resource_id;
  return resource_id;
}

const char kDocsListScope[] = "https://docs.google.com/feeds/";
const char kDriveAppsScope[] = "https://www.googleapis.com/auth/drive.apps";

void ParseShareUrlAndRun(const google_apis::GetShareUrlCallback& callback,
                         google_apis::GDataErrorCode error,
                         scoped_ptr<base::Value> value) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  if (!value) {
    callback.Run(error, GURL());
    return;
  }

  // Parsing ResourceEntry is cheap enough to do on UI thread.
  scoped_ptr<google_apis::ResourceEntry> entry =
      google_apis::ResourceEntry::ExtractAndParse(*value);
  if (!entry) {
    callback.Run(google_apis::GDATA_PARSE_ERROR, GURL());
    return;
  }

  const google_apis::Link* share_link =
      entry->GetLinkByType(google_apis::Link::LINK_SHARE);
  callback.Run(error, share_link ? share_link->href() : GURL());
}

scoped_ptr<google_apis::FileResource> ConvertResourceEntryToFileResource(
    const google_apis::ResourceEntry& entry) {
  scoped_ptr<google_apis::FileResource> file(new google_apis::FileResource);

  file->set_file_id(entry.resource_id());
  file->set_title(entry.title());
  file->set_created_date(entry.published_time());

  if (std::find(entry.labels().begin(), entry.labels().end(),
                "shared-with-me") == entry.labels().end()) {
    // Set current time to mark the file is shared_with_me, since ResourceEntry
    // doesn't have |shared_with_me_date| equivalent.
    file->set_shared_with_me_date(base::Time::Now());
  }

  file->set_download_url(entry.download_url());
  file->set_mime_type(entry.content_mime_type());

  file->set_md5_checksum(entry.file_md5());
  file->set_file_size(entry.file_size());

  file->mutable_labels()->set_trashed(entry.deleted());
  file->set_etag(entry.etag());

  ScopedVector<google_apis::ParentReference> parents;
  for (size_t i = 0; i < entry.links().size(); ++i) {
    using google_apis::Link;
    const Link& link = *entry.links()[i];
    switch (link.type()) {
      case Link::LINK_PARENT: {
        scoped_ptr<google_apis::ParentReference> parent(
            new google_apis::ParentReference);
        parent->set_parent_link(link.href());

        std::string file_id =
            drive::util::ExtractResourceIdFromUrl(link.href());
        parent->set_is_root(file_id == kWapiRootDirectoryResourceId);
        parents.push_back(parent.release());
        break;
      }
      case Link::LINK_EDIT:
        file->set_self_link(link.href());
        break;
      case Link::LINK_THUMBNAIL:
        file->set_thumbnail_link(link.href());
        break;
      case Link::LINK_ALTERNATE:
        file->set_alternate_link(link.href());
        break;
      case Link::LINK_EMBED:
        file->set_embed_link(link.href());
        break;
      default:
        break;
    }
  }
  file->set_parents(parents.Pass());

  file->set_modified_date(entry.updated_time());
  file->set_last_viewed_by_me_date(entry.last_viewed_time());

  return file.Pass();
}

scoped_ptr<google_apis::ResourceEntry>
ConvertFileResourceToResourceEntry(
    const google_apis::FileResource& file_resource) {
  scoped_ptr<google_apis::ResourceEntry> entry(new google_apis::ResourceEntry);

  // ResourceEntry
  entry->set_resource_id(file_resource.file_id());
  entry->set_id(file_resource.file_id());
  entry->set_kind(file_resource.GetKind());
  entry->set_title(file_resource.title());
  entry->set_published_time(file_resource.created_date());
  // TODO(kochi): entry->labels_
  if (!file_resource.shared_with_me_date().is_null()) {
    std::vector<std::string> labels;
    labels.push_back("shared-with-me");
    entry->set_labels(labels);
  }

  // This should be the url to download the file_resource.
  {
    google_apis::Content content;
    content.set_url(file_resource.download_url());
    content.set_mime_type(file_resource.mime_type());
    entry->set_content(content);
  }
  // TODO(kochi): entry->resource_links_

  // For file entries
  entry->set_filename(file_resource.title());
  entry->set_suggested_filename(file_resource.title());
  entry->set_file_md5(file_resource.md5_checksum());
  entry->set_file_size(file_resource.file_size());

  // If file is removed completely, that information is only available in
  // ChangeResource, and is reflected in |removed_|. If file is trashed, the
  // file entry still exists but with its "trashed" label true.
  entry->set_deleted(file_resource.labels().is_trashed());

  // CommonMetadata
  entry->set_etag(file_resource.etag());
  // entry->authors_
  // entry->links_.
  ScopedVector<google_apis::Link> links;
  if (!file_resource.parents().empty()) {
    google_apis::Link* link = new google_apis::Link;
    link->set_type(google_apis::Link::LINK_PARENT);
    link->set_href(file_resource.parents()[0]->parent_link());
    links.push_back(link);
  }
  if (!file_resource.self_link().is_empty()) {
    google_apis::Link* link = new google_apis::Link;
    link->set_type(google_apis::Link::LINK_EDIT);
    link->set_href(file_resource.self_link());
    links.push_back(link);
  }
  if (!file_resource.thumbnail_link().is_empty()) {
    google_apis::Link* link = new google_apis::Link;
    link->set_type(google_apis::Link::LINK_THUMBNAIL);
    link->set_href(file_resource.thumbnail_link());
    links.push_back(link);
  }
  if (!file_resource.alternate_link().is_empty()) {
    google_apis::Link* link = new google_apis::Link;
    link->set_type(google_apis::Link::LINK_ALTERNATE);
    link->set_href(file_resource.alternate_link());
    links.push_back(link);
  }
  if (!file_resource.embed_link().is_empty()) {
    google_apis::Link* link = new google_apis::Link;
    link->set_type(google_apis::Link::LINK_EMBED);
    link->set_href(file_resource.embed_link());
    links.push_back(link);
  }
  entry->set_links(links.Pass());

  // entry->categories_
  entry->set_updated_time(file_resource.modified_date());
  entry->set_last_viewed_time(file_resource.last_viewed_by_me_date());

  entry->FillRemainingFields();
  return entry.Pass();
}

scoped_ptr<google_apis::ResourceEntry>
ConvertChangeResourceToResourceEntry(
    const google_apis::ChangeResource& change_resource) {
  scoped_ptr<google_apis::ResourceEntry> entry;
  if (change_resource.file())
    entry = ConvertFileResourceToResourceEntry(*change_resource.file()).Pass();
  else
    entry.reset(new google_apis::ResourceEntry);

  entry->set_resource_id(change_resource.file_id());
  // If |is_deleted()| returns true, the file is removed from Drive.
  entry->set_removed(change_resource.is_deleted());
  entry->set_changestamp(change_resource.change_id());

  return entry.Pass();
}

scoped_ptr<google_apis::ResourceList>
ConvertFileListToResourceList(const google_apis::FileList& file_list) {
  scoped_ptr<google_apis::ResourceList> feed(new google_apis::ResourceList);

  const ScopedVector<google_apis::FileResource>& items = file_list.items();
  ScopedVector<google_apis::ResourceEntry> entries;
  for (size_t i = 0; i < items.size(); ++i)
    entries.push_back(ConvertFileResourceToResourceEntry(*items[i]).release());
  feed->set_entries(entries.Pass());

  ScopedVector<google_apis::Link> links;
  if (!file_list.next_link().is_empty()) {
    google_apis::Link* link = new google_apis::Link;
    link->set_type(google_apis::Link::LINK_NEXT);
    link->set_href(file_list.next_link());
    links.push_back(link);
  }
  feed->set_links(links.Pass());

  return feed.Pass();
}

scoped_ptr<google_apis::ResourceList>
ConvertChangeListToResourceList(const google_apis::ChangeList& change_list) {
  scoped_ptr<google_apis::ResourceList> feed(new google_apis::ResourceList);

  const ScopedVector<google_apis::ChangeResource>& items = change_list.items();
  ScopedVector<google_apis::ResourceEntry> entries;
  for (size_t i = 0; i < items.size(); ++i) {
    entries.push_back(
        ConvertChangeResourceToResourceEntry(*items[i]).release());
  }
  feed->set_entries(entries.Pass());

  feed->set_largest_changestamp(change_list.largest_change_id());

  ScopedVector<google_apis::Link> links;
  if (!change_list.next_link().is_empty()) {
    google_apis::Link* link = new google_apis::Link;
    link->set_type(google_apis::Link::LINK_NEXT);
    link->set_href(change_list.next_link());
    links.push_back(link);
  }
  feed->set_links(links.Pass());

  return feed.Pass();
}

const char kWapiRootDirectoryResourceId[] = "folder:root";

}  // namespace util
}  // namespace drive
