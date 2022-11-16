/* Cydia - iPhone UIKit Front-End for Debian APT
 * Copyright (C) 2008-2015  Jay Freeman (saurik)
 */

/* GNU General Public License, Version 3 {{{ */
/*
 * Cydia is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * Cydia is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cydia.  If not, see <http://www.gnu.org/licenses/>.
 **/
/* }}} */

#include "CyteKit/UCPlatform.h"

#include <cstdio>
#include <cstdlib>

#include <errno.h>
#include <sysexits.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <launch.h>

#include <sys/stat.h>

#include <Menes/Function.h>

#include <spawn.h>
extern "C"
{
    int posix_spawnattr_set_persona_np(const posix_spawnattr_t *__restrict, uid_t, uint32_t);
    int posix_spawnattr_set_persona_uid_np(const posix_spawnattr_t *__restrict, uid_t);
    int posix_spawnattr_set_persona_gid_np(const posix_spawnattr_t *__restrict, uid_t);

    int proc_pidpath(int pid, void *buffer, uint32_t buffersize);
}

#define POSIX_SPAWN_PERSONA_FLAGS_OVERRIDE 1
#define PROC_PIDPATHINFO_MAXSIZE 1024

#include <dlfcn.h>
#define CYDIA_PATH "/Applications/Cydia.app/Cydia"

int posix_spawn_root(int argc, char *argv[], char *envp[])
{
    posix_spawnattr_t attr;
    posix_spawnattr_init(&attr);
    posix_spawnattr_set_persona_np(&attr, /*persona_id=*/99, POSIX_SPAWN_PERSONA_FLAGS_OVERRIDE);
    posix_spawnattr_set_persona_uid_np(&attr, 0);
    posix_spawnattr_set_persona_gid_np(&attr, 0);
    int pid = 0;
    int status = 0;
    int ret = posix_spawnp(&pid, "/usr/libexec/cydia/cydo", NULL, &attr, argv, envp);
    if (ret)
    {
        fprintf(stderr, "cannot posix_spawnp(), errno=%d\n", ret);
        return -1;
    }
    waitpid(pid, &status, 0);
    if (WIFEXITED(status))
    {
        exit(WEXITSTATUS(status));
    }
    else if (WIFSIGNALED(status))
    {
        fprintf(stderr, "child exited due to signal: %d\n", WTERMSIG(status));
        exit(WTERMSIG(status));
    }
    return -1;
}

int main(int argc, char *argv[], char *envp[])
{

    if (getuid() != 0)
    {
        struct stat stat_buf = {0};
        if (lstat(CYDIA_PATH, &stat_buf) == -1)
        {
            fprintf(stderr, "none shall pass\n");
            return EX_NOPERM;
        }

        pid_t parent_pid = getppid();
        char parent_path[PROC_PIDPATHINFO_MAXSIZE] = {0};
        int ret = proc_pidpath(parent_pid, parent_path, sizeof(parent_path));

        if (ret <= 0)
        {
            fprintf(stderr, "try harder\n");
            return EX_NOPERM;
        }

        if (strcmp(parent_path, CYDIA_PATH) != 0)
        {
            fprintf(stderr, "nope\n");
            return EX_NOPERM;
        }
    }

    if (geteuid() != 0)
    {
        posix_spawn_root(argc, argv, envp);
        return -1;
    }

    setuid(0);
    setgid(0);

    if (argc < 2 || argv[1][0] != '/')
        argv[0] = "/usr/bin/dpkg";
    else
    {
        --argc;
        ++argv;
    }

    execv(argv[0], argv);
    return EX_UNAVAILABLE;
}
