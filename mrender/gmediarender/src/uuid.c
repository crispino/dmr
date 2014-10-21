#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/ioctl.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <net/if.h> 
#include <errno.h>

#include "uuid.h"
#include "debug.h"


static void get_current_time(uuid_time_t *timestamp)
{

	struct timeval tp;
	
#if 0
	gettimeofday(&tp, (struct timezone *)0);
	*timestamp = (uuid_time_t) (tp.tv_sec * 10000000 + tp.tv_usec * 10 +
		0x01B21DD213814000LL);
#else
	memset(timestamp, 0x1, sizeof(uuid_time_t));
#endif
	return;
}

static uint16_t get_clock_seq(void)
{
#if 0
	return (uint16_t) (rand());
#else
	return 0;
#endif
}

static void get_mac_node(uuid_node_t *node)
{
	struct ifreq  ifreq; 
    int   sock = -1; 

	memset(&ifreq, 0, sizeof(ifreq));
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
    	debug_printf(MSG_ERROR, "[%s:%d]socket failed: %s\n", __FUNCTION__, __LINE__, strerror(errno));
        return; 
    } 
	
    strcpy(ifreq.ifr_name, "br0"); 
    if (ioctl(sock, SIOCGIFHWADDR, &ifreq) < 0) 
    { 
    	debug_printf(MSG_ERROR, "[%s:%d]ioctl failed: %s\n",  __FUNCTION__, __LINE__, strerror(errno));
    	close(sock);
        return; 
    } 
	memcpy(node->nodeID, ifreq.ifr_hwaddr.sa_data, 6);

	close(sock);
    return;
}


static void format_uuid_v1(uuid_upnp * uid,
		    uint16_t clock_seq,
		    uuid_time_t timestamp, uuid_node_t node)
{
	uid->time_low = (uint32_t)(timestamp & 0xFFFFFFFF);
	uid->time_mid = (uint16_t)((timestamp >> 32) & 0xFFFF);
	uid->time_hi_and_version = (uint16_t)((timestamp >> 48) & 0x0FFF);
	uid->time_hi_and_version |= (1 << 12);
	uid->clock_seq_low = (uint8_t) (clock_seq & 0xFF);
	uid->clock_seq_hi_and_reserved = (uint8_t) ((clock_seq & 0x3F00) >> 8);
	uid->clock_seq_hi_and_reserved |= 0x80;
	memcpy(&uid->mac, &node, sizeof uid->mac);
}

void uuid_unpack(uuid_upnp * u, char *out)
{
	sprintf(out,
		"%8.8x-%4.4x-%4.4x-%2.2x%2.2x-%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x",
		(unsigned int)u->time_low,
		u->time_mid,
		u->time_hi_and_version,
		u->clock_seq_hi_and_reserved,
		u->clock_seq_low,
		u->mac[0],
		u->mac[1], u->mac[2], u->mac[3], u->mac[4], u->mac[5]);
}


static void uuid_create(uuid_upnp * uid)
{
	uuid_time_t timestamp;
	uint16_t clockseq;
	uuid_node_t node;


	/* get current time. */
	get_current_time(&timestamp);
	
	/* get clock seq. */
	clockseq = get_clock_seq();

	/* get node ID. */
	get_mac_node(&node);

	/* stuff fields into the UUID. */
	format_uuid_v1(uid, clockseq, timestamp, node);

	return;
}



void device_uuid_create(char *device_uuid)
{
	uuid_upnp uuid;
	
	memset(&uuid, 0, sizeof(uuid));
	uuid_create(&uuid);
	uuid_unpack(&uuid, device_uuid);
}

