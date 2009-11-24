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

#define DFLT_AF "inet"

#define DNSMASQ_EXTRA_PATH		"/tmp/pmnetconfig/dnsmasq.server.conf"
#define DNSMASQ_ENABLED			"interface=usb0\n" \
	"interface=eth0\n" \
	"interface=bsl0\n" \
	"dhcp-range=192.168.0.50,192.168.0.150,12h\n"
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
#include <net/if.h>
#include <netinet/in.h>

#include "sockets.h"
#include "interface.h"
#include "net-support.h"

#include <glib.h>
#include <lunaservice.h>

extern struct aftype inet_aftype;

LSPalmService *serviceHandle;

char *tmpPath;

bool monitor_ip_forward;

int ev_size = sizeof(struct inotify_event);
int buf_size = 32*(sizeof(struct inotify_event)+16);

char *usb0[] = {"default", "dev", "usb0", 0};
char *eth0[] = {"default", "dev", "eth0", 0};
char *bsl0[] = {"default", "dev", "bsl0", 0};

int dirty_shit() {
	int ret = 0;
	ret += system("iptables -t nat -A POSTROUTING -o ppp0 -j MASQUERADE");
	setroute_init();
	getroute_init();
	ret += route_edit(RTACTION_DEL, "inet", 0, usb0);
	ret += route_edit(RTACTION_DEL, "inet", 0, eth0);
	ret += route_edit(RTACTION_DEL, "inet", 0, bsl0);

	return ret;
}

int set_ip_using(const char *name, int c, unsigned long ip) {

	struct ifreq ifr;
	struct sockaddr_in sin;

	safe_strncpy(ifr.ifr_name, name, IFNAMSIZ);
	memset(&sin, 0, sizeof(struct sockaddr));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = ip;
	memcpy(&ifr.ifr_addr, &sin, sizeof(struct sockaddr));
	if (ioctl(skfd, c, &ifr) < 0)
		return -1;
	return 0;

}

char *get_if_addy(char *ifname) {

	struct aftype *ap;
	struct ifreq ifr;
	struct interface *ife = 0;

	safe_strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	ife = lookup_interface(ifname);

	if (ife && do_if_fetch(ife)>=0) {

		ap = get_afntype(ife->addr.sa_family);
		if (ap == NULL)
			ap = get_afntype(0);

		if (ife->has_ip) {
			return ap->sprint(&ife->addr, 1);
		}
	}

	return NULL;

}

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

bool get_ip_address(LSHandle* lshandle, LSMessage *message, void *ctx) {

	LSError lserror;
	LSErrorInit(&lserror);

	json_t *object = LSMessageGetPayloadJSON(message);

	char *interface = 0;
	json_get_string(object, "interface", &interface);

	char *address = get_if_addy(interface);

	int len = 0;
	char *jsonResponse = 0;
	asprintf(&jsonResponse, "{\"interface\":\"%s\",\"address\":\"%s\"}", interface, address);

	if (jsonResponse) {
		LSMessageReply(lshandle,message,jsonResponse,&lserror);
		free(jsonResponse);
	} else
		LSMessageReply(lshandle,message,"{\"returnValue\":false}",&lserror);

	if (LSErrorIsSet(&lserror)) {
		LSErrorPrint(&lserror, stderr);
		LSErrorFree(&lserror);
	}

	return true;

}

bool set_ip_address(LSHandle* lshandle, LSMessage *message, void *ctx) {

	LSError lserror;
	LSErrorInit(&lserror);

	json_t *object = LSMessageGetPayloadJSON(message);

	int ret = -1;
	char *interface = 0;
	char *address = 0;
	json_get_string(object, "interface", &interface);
	json_get_string(object, "address", &address);

	unsigned long ip;
	struct sockaddr_in sin;
	char host[128];
	safe_strncpy(host, address, (sizeof host));
	inet_aftype.input(0, host, (struct sockaddr *)&sin);
	memcpy(&ip, &sin.sin_addr.s_addr, sizeof(unsigned long));
	ret = set_ip_using(interface, SIOCSIFADDR, ip);

	int len = 0;
	char *jsonResponse = 0;
	asprintf(&jsonResponse, "{\"interface\":\"%s\",\"address\":\"%s\"}", interface, get_if_addy(interface));

	if (jsonResponse) {
		LSMessageReply(lshandle,message,jsonResponse,&lserror);
		free(jsonResponse);
	} else
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
		{"get_ip_address",		get_ip_address},
		{"set_ip_address",		set_ip_address},
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

	skfd = sockets_open(0);

	int ret = 0;

	do {
		ret = umount(IP_FORWARD);
	} while (ret==0);

	int len = 0;
	struct dirent *dp;
	DIR *tmp = opendir("/tmp");
	char *sub;
	char *path;
	while ((dp = readdir(tmp)) != NULL) {
		sub = substr(dp->d_name,0,11);
		if (strcmp(sub,"freeTether.")==0) {
			path = 0;
			len = asprintf(&path, "/tmp/%s", dp->d_name);
			if (path) {
				umount(path);
				rmdir(path);
				free(path);
			}
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

	dirty_shit();

	start_service();

	monitor_ip_forward = false;

	umount(tmpDir);
	rmdir(tmpDir);

	umount(IP_FORWARD);

	close(skfd);

	return 0;

}
