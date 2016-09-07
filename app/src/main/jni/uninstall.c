#include <jni.h>
#include <android/log.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>

static const char APP_DIR[] =
        "/data/data/com.jojo.jnitest";
static const char APP_FILES_DIR[] =
        "/data/data/com.jojo.jnitest/uninstall";
static const char APP_LOCK_FILE[] =
        "/data/data/com.jojo.jnitest/uninstall/lockFile";
static const char APP_OBSERVED_FILE[] =
        "/data/data/com.jojo.jnitest/uninstall/observedFile";
static const char APP_UNINSTALL_PID_FILE[] =
        "/data/data/com.jojo.jnitest/uninstall/uninstall"; // 存储c层监听进程的pid
static const char *kTAG = "uninstall";

#define LOGI(...)   ((void)__android_log_print(ANDROID_LOG_INFO, kTAG, __VA_ARGS__))
#define LOGE(...)   ((void)__android_log_print(ANDROID_LOG_ERROR, kTAG, __VA_ARGS__))


JNIEXPORT jstring JNICALL
Java_com_jojo_jnitest_UninstallUtil_getMsgFromJni(JNIEnv *env, jobject instance) {
    #if defined(__arm__)
    #if defined(__ARM_ARCH_7A__)
    #if defined(__ARM_NEON__)
    #if defined(__ARM_PCS_VFP)
    #define ABI "armeabi-v7a/NEON (hard-float)"
    #else
    #define ABI "armeabi-v7a/NEON"
    #endif
    #else
    #if defined(__ARM_PCS_VFP)
    #define ABI "armeabi-v7a (hard-float)"
    #else
    #define ABI "armeabi-v7a"
    #endif
    #endif
    #else
    #define ABI "armeabi"
    #endif
    #elif defined(__i386__)
    #define ABI "x86"
    #elif defined(__x86_64__)
    #define ABI "x86_64"
    #elif defined(__mips64)  /* mips64el-* toolchain defines __mips__ too */
    #define ABI "mips64"
    #elif defined(__mips__)
    #define ABI "mips"
    #elif defined(__aarch64__)
    #define ABI "arm64-v8a"
    #else
    #define ABI "unknown"
    #endif

    LOGI("hello, wo shi  rizhi a");
    return (*env)->NewStringUTF(env, "current device abi : " ABI ".");
}

JNIEXPORT void JNICALL
Java_com_jojo_jnitest_UninstallUtil_init(JNIEnv *env, jclass type, jstring intentAction_,
                                         jstring url_, jint version) {
    LOGI("init");
    // fork子进程，以执行轮询任务，pid_t的定义在<sys/stat.h>，然后fork()在<fcntl.h>中定义
    pid_t fpid = fork();
    if(fpid < 0){
        LOGE("fork an pid failed.");
    }else if (fpid == 0){
        LOGI("I'm the forked pid, i will monitor ..., id = " + getpid());
        // 若监听文件所在文件夹不存在，创建
        FILE *p_filesDir = fopen(APP_FILES_DIR, "r");
        if (p_filesDir == NULL){
            int filesDirRet = mkdir(APP_FILES_DIR, S_IRWXU | S_IRWXG | S_IXOTH);
            if (filesDirRet == -1) {
                LOGE("create dir failed");
                // 退出进程
                exit(1);
            }
        }

        // 若被监听文件不存在，创建文件
        FILE *p_observedFile = fopen(APP_OBSERVED_FILE, "r");
        if (p_observedFile == NULL) {
            p_observedFile = fopen(APP_OBSERVED_FILE, "w");
        }
        fclose(p_observedFile);

        // 创建锁文件，通过检测加锁状态来保证只有一个卸载监听进程
        int lockFileDescriptor = open(APP_LOCK_FILE, O_RDONLY);
        if (lockFileDescriptor == -1) {
            lockFileDescriptor = open(APP_LOCK_FILE, O_CREAT);
        }
        int lockRet = flock(lockFileDescriptor, LOCK_EX | LOCK_NB);
        if (lockRet == -1) {
            LOGE("observed by another process");
            exit(0);
        }
        LOGI("observed by child process");

        // 把c层监听进程的pid写入APP_UNINSTALL_PID_FILE
        FILE *uninstallFile = fopen(APP_UNINSTALL_PID_FILE, "w");
        fprintf(uninstallFile, "%d", getpid());
        fclose(uninstallFile);

        //子进程注册目录监听器
        int fileDescriptor = inotify_init();
        if (fileDescriptor < 0) {
            LOGE("inotify_init failed !!!");
            exit(1);
        }
        int watchDescriptor;
        watchDescriptor = inotify_add_watch(fileDescriptor, APP_OBSERVED_FILE, IN_DELETE);
        if (watchDescriptor < 0) {
            LOGE("inotify_add_watch failed !!!");
            exit(1);
        }

        //分配缓存，以便读取event，缓存大小=一个struct inotify_event的大小，这样一次处理一个event
        void *p_buf = malloc(sizeof(struct inotify_event));
        if (p_buf == NULL) {
            LOGE("malloc failed !!!");
            exit(1);
        }

        //开始监听
        LOGI("start observer");
        //read会阻塞进程，
        size_t readBytes = read(fileDescriptor, p_buf, sizeof(struct inotify_event));
        sleep(1); // 睡眠一秒钟，让系统删除干净包名。(防止监听到observedFile被删除后，而包目录还未删除，代码就已经执行到此处。导致不弹出浏览器)
        //走到这里说明收到目录被删除的事件，注销监听器
        free(p_buf);
        inotify_rm_watch(fileDescriptor, IN_DELETE);

        // 判断是否真正卸载了
        FILE *p_appDir = fopen(APP_DIR, "r");
        // 确认已卸载
        if (p_appDir == NULL) {
            //目录不存在log
            LOGI("uninstalled");
            if (version >= 17) {
                //4.2以上的系统由于用户权限管理更严格，需要加上 --user 0
                if (strcmp((*env)->GetStringUTFChars(env, intentAction_, NULL),
                           "android.intent.action.VIEW") == 0) {
                    // 系统没有自带浏览器，启动默认浏览器
                    LOGI("Android big than 17，default");
                    execlp("am", "am", "start", "--user", "0", "-a",
                           "android.intent.action.VIEW", "-d",
                           (*env)->GetStringUTFChars(env, url_, NULL),
                           (char *) NULL);
                } else {
                    //启动自带浏览器
                    LOGI("Android big than 17，self ");
                    execlp("am", "am", "start", "--user", "0", "-a",
                           "android.intent.action.VIEW", "-d",
                           (*env)->GetStringUTFChars(env, url_, NULL), "-n",
                           "com.android.browser/com.android.browser.BrowserActivity",
                           (char *) NULL);
                }
            } else {
                if (strcmp((*env)->GetStringUTFChars(env, intentAction_, NULL),
                           "android.intent.action.VIEW") == 0) {
                    //系统没有自带浏览器，启动默认浏览器
                    LOGI("Android small than 17，default ");
                    execlp("am", "am", "start", "-a",
                           "android.intent.action.VIEW", "-d",
                           (*env)->GetStringUTFChars(env, url_, NULL),
                           (char *) NULL);
                } else {
                    //启动自带浏览器
                    LOGI("Android small than 17，self");
                    execlp("am", "am", "start", "-a",
                           "android.intent.action.VIEW", "-d",
                           (*env)->GetStringUTFChars(env, url_, NULL), "-n",
                           "com.android.browser/com.android.browser.BrowserActivity",
                           (char *) NULL);
                }
            }

        } else {
            //如果observedFile被删除了，但是实际未卸载，则不弹出浏览器。也防止了一些误弹浏览器操作。
        }
    } else{
        //父进程直接退出，使子进程被init进程领养，以避免子进程僵死
        LOGI("ha, I'm parent process");
    }
}