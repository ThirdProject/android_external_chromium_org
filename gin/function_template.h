// This file was GENERATED by command:
//     pump.py function_template.h.pump
// DO NOT EDIT BY HAND!!!



// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/callback.h"
#include "base/logging.h"
#include "gin/arguments.h"
#include "gin/converter.h"
#include "gin/public/gin_embedders.h"
#include "gin/public/wrapper_info.h"
#include "gin/wrappable.h"

#include "v8/include/v8.h"

namespace gin {

class PerIsolateData;

namespace internal {

template<typename T>
struct RemoveConstRef {
  typedef T Type;
};
template<typename T>
struct RemoveConstRef<const T&> {
  typedef T Type;
};


// CallbackHolder and CallbackHolderBase are used to pass a base::Callback from
// CreateFunctionTemplate through v8 (via v8::FunctionTemplate) to
// DispatchToCallback, where it is invoked.
//
// v8::FunctionTemplate only supports passing void* as data so how do we know
// when to delete the base::Callback? That's where CallbackHolderBase comes in.
// It inherits from Wrappable, which delete itself when both (a) the refcount
// via base::RefCounted has dropped to zero, and (b) there are no more
// JavaScript references in V8.
class CallbackHolderBase : public Wrappable {
 public:
  virtual WrapperInfo* GetWrapperInfo() OVERRIDE;
  static WrapperInfo kWrapperInfo;
 protected:
  virtual ~CallbackHolderBase() {}
};

template<typename Sig>
class CallbackHolder : public CallbackHolderBase {
 public:
  CallbackHolder(const base::Callback<Sig>& callback)
      : callback(callback) {}
  base::Callback<Sig> callback;
 private:
  virtual ~CallbackHolder() {}
};


// This set of templates invokes a base::Callback, converts the return type to a
// JavaScript value, and returns that value to script via the provided
// gin::Arguments object.
//
// In C++, you can declare the function foo(void), but you can't pass a void
// expression to foo. As a result, we must specialize the case of Callbacks that
// have the void return type.
template<typename R, typename P1 = void, typename P2 = void, typename P3 = void>
struct Invoker {
  inline static void Go(
      Arguments* args,
      const base::Callback<R(P1, P2, P3)>& callback,
      const P1& a1,
      const P2& a2,
      const P3& a3) {
    args->Return(callback.Run(a1, a2, a3));
  }
};
template<typename P1, typename P2, typename P3>
struct Invoker<void, P1, P2, P3> {
  inline static void Go(
      Arguments* args,
      const base::Callback<void(P1, P2, P3)>& callback,
      const P1& a1,
      const P2& a2,
      const P3& a3) {
    callback.Run(a1, a2, a3);
  }
};

template<typename R, typename P1, typename P2>
struct Invoker<R, P1, P2, void> {
  inline static void Go(
      Arguments* args,
      const base::Callback<R(P1, P2)>& callback,
      const P1& a1,
      const P2& a2) {
    args->Return(callback.Run(a1, a2));
  }
};
template<typename P1, typename P2>
struct Invoker<void, P1, P2, void> {
  inline static void Go(
      Arguments* args,
      const base::Callback<void(P1, P2)>& callback,
      const P1& a1,
      const P2& a2) {
    callback.Run(a1, a2);
  }
};

template<typename R, typename P1>
struct Invoker<R, P1, void, void> {
  inline static void Go(
      Arguments* args,
      const base::Callback<R(P1)>& callback,
      const P1& a1) {
    args->Return(callback.Run(a1));
  }
};
template<typename P1>
struct Invoker<void, P1, void, void> {
  inline static void Go(
      Arguments* args,
      const base::Callback<void(P1)>& callback,
      const P1& a1) {
    callback.Run(a1);
  }
};

template<typename R>
struct Invoker<R, void, void, void> {
  inline static void Go(
      Arguments* args,
      const base::Callback<R()>& callback) {
    args->Return(callback.Run());
  }
};
template<>
struct Invoker<void, void, void, void> {
  inline static void Go(
      Arguments* args,
      const base::Callback<void()>& callback) {
    callback.Run();
  }
};


// DispatchToCallback converts all the JavaScript arguments to C++ types and
// invokes the base::Callback.
template<typename R>
static void DispatchToCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  Arguments args(info);
  CallbackHolderBase* holder_base = NULL;
  CHECK(args.GetData(&holder_base));

  typedef CallbackHolder<R()> HolderT;
  HolderT* holder = static_cast<HolderT*>(holder_base);

  Invoker<R>::Go(&args, holder->callback);
}

template<typename R, typename P1>
static void DispatchToCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  Arguments args(info);
  CallbackHolderBase* holder_base = NULL;
  CHECK(args.GetData(&holder_base));

  typedef CallbackHolder<R(P1)> HolderT;
  HolderT* holder = static_cast<HolderT*>(holder_base);

  typename RemoveConstRef<P1>::Type a1;
  if (!args.GetNext(&a1)) {
    args.ThrowError();
    return;
  }

  Invoker<R, P1>::Go(&args, holder->callback, a1);
}

template<typename R, typename P1, typename P2>
static void DispatchToCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  Arguments args(info);
  CallbackHolderBase* holder_base = NULL;
  CHECK(args.GetData(&holder_base));

  typedef CallbackHolder<R(P1, P2)> HolderT;
  HolderT* holder = static_cast<HolderT*>(holder_base);

  typename RemoveConstRef<P1>::Type a1;
  typename RemoveConstRef<P2>::Type a2;
  if (!args.GetNext(&a1) ||
      !args.GetNext(&a2)) {
    args.ThrowError();
    return;
  }

  Invoker<R, P1, P2>::Go(&args, holder->callback, a1, a2);
}

template<typename R, typename P1, typename P2, typename P3>
static void DispatchToCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  Arguments args(info);
  CallbackHolderBase* holder_base = NULL;
  CHECK(args.GetData(&holder_base));

  typedef CallbackHolder<R(P1, P2, P3)> HolderT;
  HolderT* holder = static_cast<HolderT*>(holder_base);

  typename RemoveConstRef<P1>::Type a1;
  typename RemoveConstRef<P2>::Type a2;
  typename RemoveConstRef<P3>::Type a3;
  if (!args.GetNext(&a1) ||
      !args.GetNext(&a2) ||
      !args.GetNext(&a3)) {
    args.ThrowError();
    return;
  }

  Invoker<R, P1, P2, P3>::Go(&args, holder->callback, a1, a2, a3);
}

}  // namespace internal


// This should be called once per-isolate to initialize the function template
// system.
void InitFunctionTemplates(PerIsolateData* isolate_data);

// This has to be outside the internal namespace because template
// specializations must be declared in the same namespace as the original
// template.
template<>
struct Converter<internal::CallbackHolderBase*>
    : public WrappableConverter<internal::CallbackHolderBase> {};


// Creates a v8::FunctionTemplate that will run the provided base::Callback each
// time it is called. JavaScript arguments and return values are converted via
// gin::Converter.
template<typename R>
v8::Local<v8::FunctionTemplate> CreateFunctionTemplate(
    v8::Isolate* isolate,
    const base::Callback<R()> callback) {
  typedef internal::CallbackHolder<R()> HolderT;
  scoped_refptr<HolderT> holder(new HolderT(callback));
  return v8::FunctionTemplate::New(
      &internal::DispatchToCallback<R>,
      ConvertToV8<internal::CallbackHolderBase*>(isolate, holder.get()));
}

template<typename R, typename P1>
v8::Local<v8::FunctionTemplate> CreateFunctionTemplate(
    v8::Isolate* isolate,
    const base::Callback<R(P1)> callback) {
  typedef internal::CallbackHolder<R(P1)> HolderT;
  scoped_refptr<HolderT> holder(new HolderT(callback));
  return v8::FunctionTemplate::New(
      &internal::DispatchToCallback<R, P1>,
      ConvertToV8<internal::CallbackHolderBase*>(isolate, holder.get()));
}

template<typename R, typename P1, typename P2>
v8::Local<v8::FunctionTemplate> CreateFunctionTemplate(
    v8::Isolate* isolate,
    const base::Callback<R(P1, P2)> callback) {
  typedef internal::CallbackHolder<R(P1, P2)> HolderT;
  scoped_refptr<HolderT> holder(new HolderT(callback));
  return v8::FunctionTemplate::New(
      &internal::DispatchToCallback<R, P1, P2>,
      ConvertToV8<internal::CallbackHolderBase*>(isolate, holder.get()));
}

template<typename R, typename P1, typename P2, typename P3>
v8::Local<v8::FunctionTemplate> CreateFunctionTemplate(
    v8::Isolate* isolate,
    const base::Callback<R(P1, P2, P3)> callback) {
  typedef internal::CallbackHolder<R(P1, P2, P3)> HolderT;
  scoped_refptr<HolderT> holder(new HolderT(callback));
  return v8::FunctionTemplate::New(
      &internal::DispatchToCallback<R, P1, P2, P3>,
      ConvertToV8<internal::CallbackHolderBase*>(isolate, holder.get()));
}

}  // namespace gin
