// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_SCOPE_H_
#define TOOLS_GN_SCOPE_H_

#include <map>
#include <set>

#include "base/basictypes.h"
#include "base/containers/hash_tables.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "tools/gn/err.h"
#include "tools/gn/pattern.h"
#include "tools/gn/source_dir.h"
#include "tools/gn/value.h"

class FunctionCallNode;
class ImportManager;
class Item;
class ParseNode;
class Settings;
class TargetManager;
class Template;

// Scope for the script execution.
//
// Scopes are nested. Writing goes into the toplevel scope, reading checks
// values resursively down the stack until a match is found or there are no
// more containing scopes.
//
// A containing scope can be const or non-const. The const containing scope is
// used primarily to refer to the master build config which is shared across
// many invocations. A const containing scope, however, prevents us from
// marking variables "used" which prevents us from issuing errors on unused
// variables. So you should use a non-const containing scope whenever possible.
class Scope {
 public:
  typedef base::hash_map<base::StringPiece, Value> KeyValueMap;
  // Holds an owning list of scoped_ptrs of Items (since we can't make a vector
  // of scoped_ptrs).
  typedef ScopedVector< scoped_ptr<Item> > ItemVector;

  // Allows code to provide values for built-in variables. This class will
  // automatically register itself on construction and deregister itself on
  // destruction.
  class ProgrammaticProvider {
   public:
    ProgrammaticProvider(Scope* scope) : scope_(scope) {
      scope_->AddProvider(this);
    }
    ~ProgrammaticProvider() {
      scope_->RemoveProvider(this);
    }

    // Returns a non-null value if the given value can be programmatically
    // generated, or NULL if there is none.
    virtual const Value* GetProgrammaticValue(
        const base::StringPiece& ident) = 0;

   protected:
    Scope* scope_;
  };

  // Creates an empty toplevel scope.
  Scope(const Settings* settings);

  // Creates a dependent scope.
  Scope(Scope* parent);
  Scope(const Scope* parent);

  ~Scope();

  const Settings* settings() const { return settings_; }

  // See the const_/mutable_containing_ var declaraions below. Yes, it's a
  // bit weird that we can have a const pointer to the "mutable" one.
  Scope* mutable_containing() { return mutable_containing_; }
  const Scope* mutable_containing() const { return mutable_containing_; }
  const Scope* const_containing() const { return const_containing_; }
  const Scope* containing() const {
    return mutable_containing_ ? mutable_containing_ : const_containing_;
  }

  // Returns NULL if there's no such value.
  //
  // counts_as_used should be set if the variable is being read in a way that
  // should count for unused variable checking.
  const Value* GetValue(const base::StringPiece& ident,
                        bool counts_as_used);
  const Value* GetValue(const base::StringPiece& ident) const;

  // Returns the requested value as a mutable one if possible. If the value
  // is not found in a mutable scope, then returns null. Note that the value
  // could still exist in a const scope, so GetValue() could still return
  // non-null in this case.
  //
  // Say you have a local scope that then refers to the const root scope from
  // the master build config. You can't change the values from the master
  // build config (it's read-only so it can be read from multiple threads
  // without locking). Read-only operations would work on values from the root
  // scope, but write operations would only work on values in the derived
  // scope(s).
  //
  // Be careful when calling this. It's not normally correct to modify values,
  // but you should instead do a new Set each time.
  //
  // Consider this code:
  //   a = 5
  //    {
  //       a = 6
  //    }
  // The 6 should get set on the nested scope rather than modify the value
  // in the outer one.
  Value* GetMutableValue(const base::StringPiece& ident, bool counts_as_used);

  // Same as GetValue, but if the value exists in a parent scope, we'll copy
  // it to the current scope. If the return value is non-null, the value is
  // guaranteed to be set in the current scope. Generatlly this will be used
  // if the calling code is planning on modifying the value in-place.
  //
  // Since this is used when doing read-modifies, we never count this access
  // as reading the variable, since we assume it will be written to.
  Value* GetValueForcedToCurrentScope(const base::StringPiece& ident,
                                      const ParseNode* set_node);

  // The set_node indicates the statement that caused the set, for displaying
  // errors later. Returns a pointer to the value in the current scope (a copy
  // is made for storage).
  Value* SetValue(const base::StringPiece& ident,
                  const Value& v,
                  const ParseNode* set_node);

  // Removes the value with the given identifier if it exists on the current
  // scope. This does not search recursive scopes. Does nothing if not found.
  void RemoveIdentifier(const base::StringPiece& ident);

  // Templates associated with this scope. A template can only be set once, so
  // AddTemplate will fail and return false if a rule with that name already
  // exists. GetTemplate returns NULL if the rule doesn't exist, and it will
  // check all containing scoped rescursively.
  bool AddTemplate(const std::string& name, const Template* templ);
  const Template* GetTemplate(const std::string& name) const;

  // Marks the given identifier as (un)used in the current scope.
  void MarkUsed(const base::StringPiece& ident);
  void MarkUnused(const base::StringPiece& ident);

  // Checks to see if the scope has a var set that hasn't been used. This is
  // called before replacing the var with a different one. It does not check
  // containing scopes.
  //
  // If the identifier is present but hasnn't been used, return true.
  bool IsSetButUnused(const base::StringPiece& ident) const;

  // Checks the scope to see if any values were set but not used, and fills in
  // the error and returns false if they were.
  bool CheckForUnusedVars(Err* err) const;

  // Returns all values set in the current scope, without going to the parent
  // scopes.
  void GetCurrentScopeValues(KeyValueMap* output) const;

  // Copies this scope's values into the destination. Values from the
  // containing scope(s) (normally shadowed into the current one) will not be
  // copied, neither will the reference to the containing scope (this is why
  // it's "non-recursive").
  //
  // If clobber_existing is true, any existing values will be overwritten. In
  // this mode, this function will never fail.
  //
  // If clobber_existing is false, it will be an error to merge a variable into
  // a scope that already has something with that name in scope (meaning in
  // that scope or in any of its containing scopes). If this happens, the error
  // will be set and the function will return false.
  //
  // This is used in different contexts. When generating the error, the given
  // parse node will be blamed, and the given desc will be used to describe
  // the operation that doesn't support doing this. For example, desc_for_err
  // would be "import" when doing an import, and the error string would say
  // something like "The import contains...".
  bool NonRecursiveMergeTo(Scope* dest,
                           bool clobber_existing,
                           const ParseNode* node_for_err,
                           const char* desc_for_err,
                           Err* err) const;

  // Constructs a scope that is a copy of the current one. Nested scopes will
  // be collapsed until we reach a const containing scope. The resulting
  // closure will reference the const containing scope as its containing scope
  // (since we assume the const scope won't change, we don't have to copy its
  // values).
  scoped_ptr<Scope> MakeClosure() const;

  // Makes an empty scope with the given name. Returns NULL if the name is
  // already set.
  Scope* MakeTargetDefaults(const std::string& target_type);

  // Gets the scope associated with the given target name, or null if it hasn't
  // been set.
  const Scope* GetTargetDefaults(const std::string& target_type) const;

  // Filter to apply when the sources variable is assigned. May return NULL.
  const PatternList* GetSourcesAssignmentFilter() const;
  void set_sources_assignment_filter(
      scoped_ptr<PatternList> f) {
    sources_assignment_filter_ = f.Pass();
  }

  // Indicates if we're currently processing the build configuration file.
  // This is true when processing the config file for any toolchain.
  //
  // To set or clear the flag, it must currently be in the opposite state in
  // the current scope. Note that querying the state of the flag recursively
  // checks all containing scopes until it reaches the top or finds the flag
  // set.
  void SetProcessingBuildConfig();
  void ClearProcessingBuildConfig();
  bool IsProcessingBuildConfig() const;

  // Indicates if we're currently processing an import file.
  //
  // See SetProcessingBaseConfig for how flags work.
  void SetProcessingImport();
  void ClearProcessingImport();
  bool IsProcessingImport() const;

  // The source directory associated with this scope. This will check embedded
  // scopes until it finds a nonempty source directory. This will default to
  // an empty dir if no containing scope has a source dir set.
  const SourceDir& GetSourceDir() const;
  void set_source_dir(const SourceDir& d) { source_dir_ = d; }

  // The item collector is where Items (Targets, Configs, etc.) go that have
  // been defined. If a scope can generate items, this non-owning pointer will
  // point to the storage for such items. The creator of this scope will be
  // responsible for setting up the collector and then dealing with the
  // collected items once execution of the context is complete.
  //
  // The items in a scope are collected as we go and then dispatched at the end
  // of execution of a scope so that we can query the previously-generated
  // targets (like getting the outputs).
  //
  // This can be null if the current scope can not generate items (like for
  // imports and such).
  //
  // When retrieving the collector, the non-const scopes are recursively
  // queried. The collector is not copied for closures, etc.
  void set_item_collector(ItemVector* collector) {
    item_collector_ = collector;
  }
  ItemVector* GetItemCollector();

  // Properties are opaque pointers that code can use to set state on a Scope
  // that it can retrieve later.
  //
  // The key should be a pointer to some use-case-specific object (to avoid
  // collisions, otherwise it doesn't matter). Memory management is up to the
  // setter. Setting the value to NULL will delete the property.
  //
  // Getting a property recursively searches all scopes, and the optional
  // |found_on_scope| variable will be filled with the actual scope containing
  // the key (if the pointer is non-NULL).
  void SetProperty(const void* key, void* value);
  void* GetProperty(const void* key, const Scope** found_on_scope) const;

 private:
  friend class ProgrammaticProvider;

  struct Record {
    Record() : used(false) {}
    Record(const Value& v) : used(false), value(v) {}

    bool used;  // Set to true when the variable is used.
    Value value;
  };

  void AddProvider(ProgrammaticProvider* p);
  void RemoveProvider(ProgrammaticProvider* p);

  // Scopes can have no containing scope (both null), a mutable containing
  // scope, or a const containing scope. The reason is that when we're doing
  // a new target, we want to refer to the base_config scope which will be read
  // by multiple threads at the same time, so we REALLY want it to be const.
  // When you jsut do a nested {}, however, we sometimes want to be able to
  // change things (especially marking unused vars).
  const Scope* const_containing_;
  Scope* mutable_containing_;

  const Settings* settings_;

  // Bits set for different modes. See the flag definitions in the .cc file
  // for more.
  unsigned mode_flags_;

  typedef base::hash_map<base::StringPiece, Record> RecordMap;
  RecordMap values_;

  // Owning pointers. Note that this can't use string pieces since the names
  // are constructed from Values which might be deallocated before this goes
  // out of scope.
  typedef base::hash_map<std::string, Scope*> NamedScopeMap;
  NamedScopeMap target_defaults_;

  // Null indicates not set and that we should fallback to the containing
  // scope's filter.
  scoped_ptr<PatternList> sources_assignment_filter_;

  // Owning pointers, must be deleted.
  typedef std::map<std::string, scoped_refptr<const Template> > TemplateMap;
  TemplateMap templates_;

  ItemVector* item_collector_;

  // Opaque pointers. See SetProperty() above.
  typedef std::map<const void*, void*> PropertyMap;
  PropertyMap properties_;

  typedef std::set<ProgrammaticProvider*> ProviderSet;
  ProviderSet programmatic_providers_;

  SourceDir source_dir_;

  DISALLOW_COPY_AND_ASSIGN(Scope);
};

#endif  // TOOLS_GN_SCOPE_H_
