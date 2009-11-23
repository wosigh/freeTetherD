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

#define IP_FORWARD		"/proc/sys/net/ipv4/ip_forward"
#define SERVICE_URI		"us.ryanhope.freeTetherD"
#define DEBUG			1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <sys/timeb.h>
#include <sys/stat.h>
#include <sys/mount.h>

#include <glib.h>
#include <lunaservice.h>

char *tmpPath;

bool toggle_ip_forward(LSHandle* lshandle, LSMessage *message, void *ctx) {

	LSError lserror;
	LSErrorInit(&lserror);

	FILE *fp;
	int len = 0, state = 0;

	fp = fopen(tmpPath, "r");
	if (fp) {
		state = fgetc(fp);
		fclose(fp);
		fp = fopen(tmpPath, "w");
		if (fp) {
			switch ((char)state) {
			case '0': fprintf(fp, "1"); break;
			case '1': fprintf(fp, "0"); break;
			}
			fclose(fp);
			fp = fopen(tmpPath, "r");
			state = fgetc(fp);
			fclose(fp);
		}
	}

	char *tmp = 0;
	len = asprintf(&tmp, "{\"returnValue\":true,\"state\":%c}", (char)state);
	if (tmp)
		LSMessageReply(lshandle,message,tmp,&lserror);
	else
		LSMessageReply(lshandle,message,"{\"returnValue\":false}",&lserror);

	if (LSErrorIsSet(&lserror)) {
		LSErrorPrint(&lserror, stderr);
		LSErrorFree(&lserror);
	}

	return true;

}

LSMethod methods[] = {
		{"toggle_ip_forward", toggle_ip_forward},
		{0,0}
};

void start_service() {

	LSError lserror;
	LSErrorInit(&lserror);

	GMainLoop *loop = g_main_loop_new(NULL, FALSE);

	LSPalmService *serviceHandle;
	LSRegisterPalmService(SERVICE_URI, &serviceHandle, &lserror);
	LSPalmServiceRegisterCategory(serviceHandle, "/", methods, NULL, NULL, NULL, &lserror);
	LSGmainAttachPalmService(serviceHandle, loop, &lserror);

	g_main_loop_run(loop);

	if (LSErrorIsSet(&lserror)) {
		LSErrorPrint(&lserror, stderr);
		LSErrorFree(&lserror);
	}

}

char *substr(const char *pstr, int start, int numchars) {
	char *pnew = malloc(numchars+1);
	strncpy(pnew, pstr + start, numchars);
	pnew[numchars] = '\0';
	return pnew;
}

int main(int argc, char *argv[]) {

	int ret = 0;

	do {
		ret = umount(IP_FORWARD);
	} while (ret==0);

	struct dirent *dp;
	DIR *tmp = opendir("/tmp");
	char *sub;
	while ((dp = readdir(tmp)) != NULL) {
		sub = substr(dp->d_name,0,11);
		if (strcmp(sub,"freeTether.")==0) {
			umount(dp->d_name);
			rmdir(dp->d_name);
		}
		free(sub);
	}
	closedir(tmp);

	char template[] = "/tmp/freeTether.XXXXXX";
	char *tmpDir = strdup(mkdtemp(template));

	ret = asprintf(&tmpPath,"%s/sys/net/ipv4/ip_forward",tmpDir);

	mount("/dev/null", IP_FORWARD, NULL, MS_BIND, NULL);
	mount("proc", tmpDir, "proc", 0, NULL);

	start_service();

	umount(tmpDir);
	rmdir(tmpDir);

	umount(IP_FORWARD);


	return 0;

}
