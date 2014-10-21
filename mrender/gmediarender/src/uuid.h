#ifndef UUID_H
#define UUID_H

typedef uint64_t uuid_time_t;
typedef struct {
	char nodeID[6];
} uuid_node_t;

typedef struct _uuid_upnp {
	uint32_t time_low;
	uint16_t time_mid;
	uint16_t time_hi_and_version;
	uint8_t  clock_seq_hi_and_reserved;
	uint8_t  clock_seq_low;
	uint8_t  mac[6];
} uuid_upnp;


void device_uuid_create(char *device_uuid);


#endif /* UUID_H */
