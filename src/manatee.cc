#include <v8.h>
#include <node.h>
#include <node_buffer.h>

// C standard library
#include <cstdlib>
#include <ctime>
#include <string.h>

#include "BarcodeScanner.h"

using namespace v8;
using namespace node;

void throw_v8_exception(Isolate* isolate, const char* str) {
  isolate->ThrowException(Exception::TypeError(String::NewFromUtf8(isolate, str)));
}

void Scan(const FunctionCallbackInfo<v8::Value>& args){
  Isolate *isolate = args.GetIsolate();
  HandleScope scope(isolate);

  if (args.Length() != 5) {
    throw_v8_exception(isolate, "Scan() takes 5 arguments.");
    return;
  }

  v8::Local<v8::Object> bufferObj = args[0]->ToObject();
  uint8_t* pixels = (uint8_t *)Buffer::Data(bufferObj);
  size_t npixels = Buffer::Length(bufferObj);
  int32_t ncols = args[1]->IntegerValue();
  int32_t nrows = args[2]->IntegerValue();
  uint32_t codeMask = args[3]->IntegerValue();
  int scanningLevel = args[4]->IntegerValue();

  if ((size_t)ncols * (size_t)nrows != npixels) {
    throw_v8_exception(isolate, "Image dimensions don't match image");
    return;
  }

  if (MWB_setActiveCodes(codeMask) != MWB_RT_OK) {
    throw_v8_exception(isolate, "Couldn't set barcode types");
    return;
  }

  MWB_setDirection(MWB_SCANDIRECTION_HORIZONTAL|MWB_SCANDIRECTION_VERTICAL);
  MWB_setScanningRect(MWB_CODE_MASK_PDF, 0, 00, 100, 100);

  if (MWB_setLevel(scanningLevel) != MWB_RT_OK) {
    throw_v8_exception(isolate, "Couldn't set scanning level");
    return;
  }

  uint8_t *p_data = NULL;
  int retval = MWB_scanGrayscaleImage(pixels, ncols, nrows, &p_data);

  if (retval <= 0) {
    args.GetReturnValue().Set(Null(isolate));
    return;
  }

  Buffer *slowBuffer = Buffer::New(retval);
  memcpy(Buffer::Data(slowBuffer), p_data, retval);

  v8::Local<v8::Object> globalObj = Context::GetCurrent()->Global();
  v8::Local<v8::Object> bufferConstructor = Local<Function>::Cast(globalObj->Get(String::New("Buffer")));
  Handle<Value> constructorArgs[3] = { slowBuffer->handle_, v8::Integer::New(retval), v8::Integer::New(0) };
  v8::Local<v8::Object> actualBuffer = bufferConstructor->NewInstance(3, constructorArgs);

  args.GetReturnValue().Set(v8::Object(isolate, actualBuffer));
}


/*Handle<Value> Scan(const Arguments& args) {
  HandleScope scope;

  if (args.Length() != 5) {
    return ThrowException(
      Exception::TypeError(String::New("scan requires 5 arguments"))
    );
  }

  Local<Object> bufferObj = args[0]->ToObject();
  uint8_t* pixels = (uint8_t *)Buffer::Data(bufferObj);
  size_t npixels = Buffer::Length(bufferObj);
  int32_t ncols = args[1]->IntegerValue();
  int32_t nrows = args[2]->IntegerValue();
  uint32_t codeMask = args[3]->IntegerValue();
  int scanningLevel = args[4]->IntegerValue();

  if ((size_t)ncols * (size_t)nrows != npixels) {
    return ThrowException(
      Exception::TypeError(String::New("Image dimensions don't match image"))
    );
  }

  if (MWB_setActiveCodes(codeMask) != MWB_RT_OK) {
    return ThrowException(
      Exception::TypeError(String::New("Couldn't set barcode types"))
    );
  }

  MWB_setDirection(MWB_SCANDIRECTION_HORIZONTAL|MWB_SCANDIRECTION_VERTICAL);
  MWB_setScanningRect(MWB_CODE_MASK_PDF, 0, 00, 100, 100);

  if (MWB_setLevel(scanningLevel) != MWB_RT_OK) {
    return ThrowException(
      Exception::TypeError(String::New("Couldn't set scanning level"))
    );
  }

  uint8_t *p_data = NULL;
  int retval = MWB_scanGrayscaleImage(pixels, ncols, nrows, &p_data);

  if (retval <= 0) {
    return scope.Close(Null());
  }

  Buffer *slowBuffer = Buffer::New(retval);
  memcpy(Buffer::Data(slowBuffer), p_data, retval);

  Local<Object> globalObj = Context::GetCurrent()->Global();
  Local<Function> bufferConstructor = Local<Function>::Cast(globalObj->Get(String::New("Buffer")));
  Handle<Value> constructorArgs[3] = { slowBuffer->handle_, v8::Integer::New(retval), v8::Integer::New(0) };
  Local<Object> actualBuffer = bufferConstructor->NewInstance(3, constructorArgs);

  return scope.Close(actualBuffer);
}*/

void Register(const FunctionCallbackInfo<v8::Value>& args){
  Isolate *isolate = args.GetIsolate();
  HandleScope scope(isolate);

  if (args.Length() != 3) {
    throw_v8_exception(isolate, "Register() takes 3 arguments.");
    return;
  }

  uint32_t codeMask = args[0]->IntegerValue();
  String::Utf8Value userName(args[1]->ToString());
  String::Utf8Value key(args[2]->ToString());

  int retval = MWB_registerCode(codeMask, *userName, *key);
  args.GetReturnValue().Set(Integer::New(isolate, retval));
}

/*
Handle<Value> Register(const Arguments& args) {
  HandleScope scope;

  if (args.Length() != 3) {
    return ThrowException(
      Exception::TypeError(String::New("register requires 3 arguments"))
    );
  }

  uint32_t codeMask = args[0]->IntegerValue();
  String::AsciiValue userName(args[1]->ToString());
  String::AsciiValue key(args[2]->ToString());

  int retval = MWB_registerCode(codeMask, *userName, *key);
  return scope.Close(Integer::New(retval));
}*/

void Version(const FunctionCallbackInfo<v8::Value>& args){
  Isolate *isolate = args.GetIsolate();
  HandleScope scope(isolate);

  char versionString[256];
  unsigned int version = MWB_getLibVersion();
  sprintf(versionString, "%i.%i.%i", (version >> 16) & 0xff,
    (version >> 8) & 0xff, (version >> 0) & 0xff);
  args.GetReturnValue().Set(String::New(isolate, versionString));
}

/*
Handle<Value> Version(const Arguments& args) {
  HandleScope scope;

  char versionString[256];
  unsigned int version = MWB_getLibVersion();
  sprintf(versionString, "%i.%i.%i", (version >> 16) & 0xff,
    (version >> 8) & 0xff, (version >> 0) & 0xff);
  return scope.Close(String::New(versionString));
}*/


void RegisterModule(Handle<Object> target) {

    // target is the module object you see when require()ing the .node file.
  NODE_SET_METHOD(target, "scan"       , Scan);
  NODE_SET_METHOD(target, "register"   , Register);
  NODE_SET_METHOD(target, "version"    , Version);
}

/*
void RegisterModule(Handle<Object> target) {

    // target is the module object you see when require()ing the .node file.
  target->Set(String::NewSymbol("scan"),
    FunctionTemplate::New(Scan)->GetFunction());
  target->Set(String::NewSymbol("register"),
    FunctionTemplate::New(Register)->GetFunction());
  target->Set(String::NewSymbol("version"),
    FunctionTemplate::New(Version)->GetFunction());
}*/

NODE_MODULE(manatee, RegisterModule);
