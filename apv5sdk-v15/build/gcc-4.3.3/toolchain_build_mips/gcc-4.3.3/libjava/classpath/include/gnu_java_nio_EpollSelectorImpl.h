/* DO NOT EDIT THIS FILE - it is machine generated */

#ifndef __gnu_java_nio_EpollSelectorImpl__
#define __gnu_java_nio_EpollSelectorImpl__

#include <jni.h>

#ifdef __cplusplus
extern "C"
{
#endif

JNIEXPORT jboolean JNICALL Java_gnu_java_nio_EpollSelectorImpl_epoll_1supported (JNIEnv *env, jclass);
JNIEXPORT jint JNICALL Java_gnu_java_nio_EpollSelectorImpl_sizeof_1struct (JNIEnv *env, jclass);
JNIEXPORT jint JNICALL Java_gnu_java_nio_EpollSelectorImpl_epoll_1create (JNIEnv *env, jclass, jint);
JNIEXPORT void JNICALL Java_gnu_java_nio_EpollSelectorImpl_epoll_1add (JNIEnv *env, jclass, jint, jint, jint);
JNIEXPORT void JNICALL Java_gnu_java_nio_EpollSelectorImpl_epoll_1modify (JNIEnv *env, jclass, jint, jint, jint);
JNIEXPORT void JNICALL Java_gnu_java_nio_EpollSelectorImpl_epoll_1delete (JNIEnv *env, jclass, jint, jint);
JNIEXPORT jint JNICALL Java_gnu_java_nio_EpollSelectorImpl_epoll_1wait (JNIEnv *env, jclass, jint, jobject, jint, jint);
JNIEXPORT jint JNICALL Java_gnu_java_nio_EpollSelectorImpl_selected_1fd (JNIEnv *env, jclass, jobject);
JNIEXPORT jint JNICALL Java_gnu_java_nio_EpollSelectorImpl_selected_1ops (JNIEnv *env, jclass, jobject);
#undef gnu_java_nio_EpollSelectorImpl_DEFAULT_EPOLL_SIZE
#define gnu_java_nio_EpollSelectorImpl_DEFAULT_EPOLL_SIZE 128L
#undef gnu_java_nio_EpollSelectorImpl_OP_ACCEPT
#define gnu_java_nio_EpollSelectorImpl_OP_ACCEPT 16L
#undef gnu_java_nio_EpollSelectorImpl_OP_CONNECT
#define gnu_java_nio_EpollSelectorImpl_OP_CONNECT 8L
#undef gnu_java_nio_EpollSelectorImpl_OP_READ
#define gnu_java_nio_EpollSelectorImpl_OP_READ 1L
#undef gnu_java_nio_EpollSelectorImpl_OP_WRITE
#define gnu_java_nio_EpollSelectorImpl_OP_WRITE 4L

#ifdef __cplusplus
}
#endif

#endif /* __gnu_java_nio_EpollSelectorImpl__ */