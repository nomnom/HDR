#include <jni.h>
#include <string.h>
#include <android/log.h>

#define DEBUG_TAG "NDK_Test"

extern "C" {
  void Java_de_visus_hdrlight_Test_run(JNIEnv * env, jobject obj, jstring logThis);
}
//        <----package----> <ac> <-func->
void Java_de_visus_hdrlight_Test_run(JNIEnv * env, jobject obj, jstring args)
{
 return;
}



