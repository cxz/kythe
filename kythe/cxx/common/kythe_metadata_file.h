/*
 * Copyright 2015 The Kythe Authors. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef KYTHE_CXX_COMMON_KYTHE_METADATA_FILE_H_
#define KYTHE_CXX_COMMON_KYTHE_METADATA_FILE_H_

#include "kythe/proto/storage.pb.h"

#include "absl/memory/memory.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/MemoryBuffer.h"
#include "rapidjson/document.h"

#include <map>
#include <memory>

namespace kythe {

class MetadataFile {
 public:
  /// \brief A single metadata rule.
  struct Rule {
    unsigned begin;        ///< Beginning of the range to match.
    unsigned end;          ///< End of the range to match.
    std::string edge_in;   ///< Edge kind to match from anchor over [begin,end).
    std::string edge_out;  ///< Edge to create.
    proto::VName vname;    ///< VName to create edge to or from.
    bool reverse_edge;     ///< If false, draw edge to vname; if true, draw
                           ///< from.
    bool generate_anchor;  ///< If this rule should generate an anchor.
    unsigned anchor_begin;  ///< The beginning of the anchor.
    unsigned anchor_end;    ///< The end of the anchor.
  };

  /// Creates a new MetadataFile from a list of rules ranging from `begin` to
  /// `end`.
  template <typename InputIterator>
  static std::unique_ptr<MetadataFile> LoadFromRules(InputIterator begin,
                                                     InputIterator end) {
    std::unique_ptr<MetadataFile> meta_file = absl::make_unique<MetadataFile>();
    for (auto rule = begin; rule != end; ++rule) {
      meta_file->rules_.emplace(rule->begin, *rule);
    }
    return meta_file;
  }

  /// Rules to apply keyed on `begin`.
  const std::multimap<unsigned, Rule>& rules() const { return rules_; }

 private:
  /// Rules to apply keyed on `begin`.
  std::multimap<unsigned, Rule> rules_;
};

/// \brief Provides interested MetadataSupport classes with the ability to
/// look up VNames for arbitrary required inputs (including metadata files).
/// \return true and merges `path`'s VName into `out` on success; false on
/// failure.
using VNameLookup =
    std::function<bool(const std::string& path, proto::VName* out)>;

/// \brief Converts from arbitrary metadata formats to those supported by Kythe.
///
/// Some metadata producers may be unable to directly generate Kythe metadata.
/// It may also be difficult to ensure that the data they do produce is
/// converted before a Kythe tool runs. This interface allows tools to
/// support arbitrary metadata formats by converting them to kythe::MetadataFile
/// instances on demand.
class MetadataSupport {
 public:
  virtual ~MetadataSupport() {}
  /// \brief Attempt to parse the file originally named `raw_filename` with
  /// decoded filename `filename` and contents in `buffer`.
  /// \return A `MetadataFile` on success; otherwise, null.
  virtual std::unique_ptr<kythe::MetadataFile> ParseFile(
      const std::string& raw_filename, const std::string& filename,
      const llvm::MemoryBuffer* buffer) {
    return nullptr;
  }

  /// \brief Use `lookup` when generating VNames (if necessary).
  virtual void UseVNameLookup(VNameLookup lookup) {}
};

/// \brief A collection of metadata support implementations.
///
/// Each `MetadataSupport` is tried in order. The first one to return
/// a non-null result from `ParseFile` is elected to provide metadata for a
/// given (`filename`, `buffer`) pair.
///
/// If the metadata file ends in .h, we assume that it is a valid C++ header
/// that begins with a comment marker followed immediately by a base64-encoded
/// buffer. We will decode and parse this buffer using the filename with the .h
/// removed (as some support implementations discriminate based on extension).
/// The comment mark, the contents of the comment, and any relevant control
/// characters must be 7-bit ASCII. The character set used for base64 encoding
/// is A-Za-z0-9+/ in that order, possibly followed by = or == for padding.
///
/// If search_string is set, `buffer` will be scanned for a comment marker
/// followed by a space followed by search_string. Should this comment be
/// found, it will be decoded as above.
///
/// If the comment is a //-style comment, the base64 string must be unbroken.
/// If the comment is a /* */-style comment, newlines (\n) are permitted.
class MetadataSupports {
 public:
  void Add(std::unique_ptr<MetadataSupport> support) {
    supports_.push_back(std::move(support));
  }

  std::unique_ptr<kythe::MetadataFile> ParseFile(
      const std::string& filename, const llvm::MemoryBuffer* buffer,
      const std::string& search_string) const;

  void UseVNameLookup(VNameLookup lookup) const;

 private:
  std::vector<std::unique_ptr<MetadataSupport>> supports_;
};

/// \brief Enables support for raw JSON-encoded metadata files.
class KytheMetadataSupport : public MetadataSupport {
 public:
  std::unique_ptr<kythe::MetadataFile> ParseFile(
      const std::string& raw_filename, const std::string& filename,
      const llvm::MemoryBuffer* buffer) override;

 private:
  /// \brief Load the JSON-encoded metadata from `json`.
  /// \return null on failure.
  static std::unique_ptr<MetadataFile> LoadFromJSON(llvm::StringRef json);
  /// \brief Load the metadata rule from `value` into the Rule `rule`.
  /// \return false on failure.
  static bool LoadMetaElement(const rapidjson::Value& value,
                              MetadataFile::Rule* rule);
};

}  // namespace kythe

#endif  // KYTHE_CXX_COMMON_KYTHE_METADATA_FILE_H_
