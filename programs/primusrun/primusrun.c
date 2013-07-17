/*
 * Copyright 2013 Andrew Cook
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(primus);

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <winbase.h>

static char LD_LIBRARY_PATH[] = "LD_LIBRARY_PATH";
static char PRIMUS_libGL[] = "/usr/$LIB/primus";
static char PRIMUS_SYNC[] = "0";
static char vblank_mode[] = "0";

int main(int argc, char *argv[])
{
    pid_t pid;
    const char *lib_path = getenv("LD_LIBRARY_PATH");
    setenv("PRIMUS_libGL", PRIMUS_libGL, 1);
    setenv("PRIMUS_SYNC", PRIMUS_SYNC, 1);
    setenv("vblank_mode", vblank_mode, 1);
    
    if(lib_path) {
        char new_path[8192] = "/usr/$LIB/primus";
        strncat(new_path, ":", 8192);
        strncat(new_path, lib_path, 8192);
        setenv(LD_LIBRARY_PATH, new_path, 1);
    } else {
        setenv(LD_LIBRARY_PATH, PRIMUS_libGL, 1);
    }
    
    //steam complains if the process doesn't run or exit normally
    if((pid = fork())) {
        int stat = 1;
        while(waitpid(pid, &stat, 0) != pid && errno != ECHILD);
        ExitProcess(stat);
    }
    
    argv[0] = "wine";
    execvp("wine", argv);
}
