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

#define DNSMASQ_EXTRA_PATH		"/tmp/pmnetconfig/dnsmasq.server.conf"
#define DNSMASQ_ENABLED			"interface=usb0\ninterface=eth0\ninterface=bsl0\n"
#define IP_FORWARD				"/proc/sys/net/ipv4/ip_forward"
#define SERVICE_URI				"us.ryanhope.freeTetherD"
#define DEBUG					1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <sys/timeb.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/inotify.h>

#include <glib.h>
#include <lunaservice.h>

LSPalmService *serviceHandle;

char *tmpPath;

bool monitor_ip_forward;

int ev_size = sizeof(struct inotify_event);
int buf_size = 32*(sizeof(struct inotify_event)+16);

void *ip_forward_monitor(void *ptr) {

	LSError lserror;
	LSErrorInit(&lserror);

	int fd = inotify_init();
	if (fd<0) {
		perror("inotify_init");
		return;
	}

	int wd = inotify_add_watch(fd, tmpPath, IN_MODIFY);
	if (wd<0) {
		perror ("inotify_add_watch");
		return;
	}

	char buf[buf_size], *tmp = 0;
	int len = 0, state = 0;
	FILE *fp;

	while (monitor_ip_forward) {
		len = read (fd, buf, buf_size);
		if (len > 0) {
			fp = fopen(tmpPath, "r");
			if (fp) {
				state = fgetc(fp);
				fclose(fp);
				len = asprintf(&tmp, "{\"state\":%c}", (char)state);
				if (tmp) {
					LSSubscriptionRespond(serviceHandle,"/get_ip_forward",tmp, &lserror);
					free(tmp);
				}
			}
		}
	}

	if (LSErrorIsSet(&lserror)) {
		LSErrorPrint(&lserror, stderr);
		LSErrorFree(&lserror);
	}

}

void start_stop_dnsmasq(char *cmd) {
	pid_t pid = vfork();
	if (pid == 0)
		execl(cmd, cmd, "-q", "dnsmasq", (char*)0);
}


void restart_dnsmasq() {
	start_stop_dnsmasq("/sbin/stop");
	start_stop_dnsmasq("/sbin/start");
}

bool enable_nat(LSHandle* lshandle, LSMessage *message, void *ctx) {

	LSError lserror;
	LSErrorInit(&lserror);

	FILE *fp;
	int len = 0;

	fp = fopen(DNSMASQ_EXTRA_PATH, "w");
	if (fp) {
		len = fprintf(fp,"%s",DNSMASQ_ENABLED);
		fclose(fp);
		restart_dnsmasq();
	}

	if (len>0)
		LSMessageReply(lshandle,message,"{\"returnValue\":true}",&lserror);
	else
		LSMessageReply(lshandle,message,"{\"returnValue\":false}",&lserror);

	if (LSErrorIsSet(&lserror)) {
		LSErrorPrint(&lserror, stderr);
		LSErrorFree(&lserror);
	}

	return true;

}

bool disable_nat(LSHandle* lshandle, LSMessage *message, void *ctx) {

	LSError lserror;
	LSErrorInit(&lserror);

	FILE *fp;
	int len = 0;

	fp = fopen(DNSMASQ_EXTRA_PATH, "w");
	if (fp) {
		len = fprintf(fp,"");
		fclose(fp);
		restart_dnsmasq();
	}

	if (len==0)
		LSMessageReply(lshandle,message,"{\"returnValue\":true}",&lserror);
	else
		LSMessageReply(lshandle,message,"{\"returnValue\":false}",&lserror);

	if (LSErrorIsSet(&lserror)) {
		LSErrorPrint(&lserror, stderr);
		LSErrorFree(&lserror);
	}

	return true;

}

bool get_ip_forward(LSHandle* lshandle, LSMessage *message, void *ctx) {

	LSError lserror;
	LSErrorInit(&lserror);

	FILE *fp;
	int len = 0, state = -1;

	fp = fopen(tmpPath, "r");
	if (fp) {
		state = fgetc(fp);
		fclose(fp);
	}

	bool subscribed = false;
	LSSubscriptionProcess(lshandle,message,&subscribed,&lserror);

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

bool toggle_ip_forward(LSHandle* lshandle, LSMessage *message, void *ctx) {

	LSError lserror;
	LSErrorInit(&lserror);

	FILE *fp;
	int len = 0, state = -1;

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
		{"get_ip_forward",		get_ip_forward},
		{"toggle_ip_forward",	toggle_ip_forward},
		{"enable_nat",			enable_nat},
		{"disable_nat",			disable_nat},
		{0,0}
};

void start_service() {

	LSError lserror;
	LSErrorInit(&lserror);

	GMainLoop *loop = g_main_loop_new(NULL, FALSE);

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

	monitor_ip_forward = true;

	pthread_t ip_forward_monitor_thread;
	ret = pthread_create(&ip_forward_monitor_thread, NULL, ip_forward_monitor, NULL);

	ret = system("iptables -t nat -A POSTROUTING -o ppp0 -j MASQUERADE");
	ret = system("ip route del default scope global dev usb0");
	ret = system("ip route del default scope global dev eth0");
	ret = system("ip route del default dev bsl0");

	start_service();

	monitor_ip_forward = false;

	umount(tmpDir);
	rmdir(tmpDir);

	umount(IP_FORWARD);

	return 0;

}
