
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_util_LinkedHashMap$1__
#define __java_util_LinkedHashMap$1__

#pragma interface

#include <java/lang/Object.h>

class java::util::LinkedHashMap$1 : public ::java::lang::Object
{

public: // actually package-private
  LinkedHashMap$1(::java::util::LinkedHashMap *, jint);
public:
  jboolean hasNext();
  ::java::lang::Object * next();
  void remove();
public: // actually package-private
  ::java::util::LinkedHashMap$LinkedHashEntry * __attribute__((aligned(__alignof__( ::java::lang::Object)))) current;
  ::java::util::LinkedHashMap$LinkedHashEntry * last;
  jint knownMod;
  ::java::util::LinkedHashMap * this$0;
private:
  jint val$type;
public:
  static ::java::lang::Class class$;
};

#endif // __java_util_LinkedHashMap$1__