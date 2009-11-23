/*=============================================================================
 Copyright (C) 2009 Ryan Hope <rmh3093@gmail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 =============================================================================*/

#define IP_FORWARD "/proc/sys/net/ipv4/ip_forward"
#define DEBUG 1

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/timeb.h>
#include <sys/stat.h>
#include <sys/mount.h>

char *tmpDir;

void setup() {

	int ret = 0, wfd = 0, w = 0;

	char template[] = "/tmp/freeTether.XXXXXX";
	tmpDir = mkdtemp(template);
	if (DEBUG) puts(tmpDir);

	do {
		ret = umount(IP_FORWARD);
	} while (ret==0);

	mount("/dev/null", IP_FORWARD, NULL, MS_BIND, NULL);
	mount("proc", tmpDir, "proc", 0, NULL);

}

void cleanup() {

	umount(tmpDir);
	rmdir(tmpDir);

	umount(IP_FORWARD);

}

int main(int argc, char *argv[]) {

	setup();

	GMainLoop *loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(loop);

	cleanup();

	return 0;

}
