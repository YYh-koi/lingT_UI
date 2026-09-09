
#include "dhcps.h"
#include "tcpip.h"
#include <string.h>
#include "project.h"
#include "wlan_setting.h"

//#define memcpy gp_memcpy
//#define memset gp_memset
//static struct dhcp_server_state dhcp_server_state_machine;
static uint8_t dhcp_server_state_machine = DHCP_SERVER_STATE_IDLE;
/* recorded the client MAC addr(default sudo mac) */
//static uint8_t dhcps_record_first_client_mac[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
/* recorded transaction ID (default sudo id)*/
static uint8_t dhcp_recorded_xid[4] = {0xff, 0xff, 0xff, 0xff};

/* UDP Protocol Control Block(PCB) */
static struct udp_pcb *dhcps_pcb;

static ip_addr_t dhcps_send_broadcast_address;
static ip_addr_t dhcps_local_address;
static ip_addr_t dhcps_local_mask;
static ip_addr_t dhcps_local_gateway;
static ip_addr_t dhcps_network_id;
static ip_addr_t dhcps_subnet_broadcast;
static ip_addr_t dhcps_allocated_client_address;
static int dhcps_addr_pool_set = 0;
static ip_addr_t dhcps_addr_pool_start;
static ip_addr_t dhcps_addr_pool_end;
#if 0
static ip_addr_t dhcps_owned_first_ip;
static ip_addr_t dhcps_owned_last_ip;
static uint8_t dhcps_num_of_available_ips;
#endif
static struct dhcp_msg *dhcp_message_repository;
static int dhcp_message_total_options_lenth;

/* allocated IP range */
static struct table  ip_table;
static ip_addr_t client_request_ip;

static xSemaphoreHandle dhcps_ip_table_semaphore;
extern unsigned int g_ap_sta_num;

static struct netif * dhcps_netif = NULL;

#ifndef ATBM_DHCPS_TRACE
#define ATBM_DHCPS_TRACE 1
#endif

#if ATBM_DHCPS_TRACE
static const char *dhcps_state_string(uint8_t state)
{
	switch (state) {
	case DHCP_SERVER_STATE_OFFER:
		return "OFFER";
	case DHCP_SERVER_STATE_ACK:
		return "ACK";
	case DHCP_SERVER_STATE_NAK:
		return "NAK";
	case DHCP_SERVER_STATE_IDLE:
		return "IDLE";
	default:
		return "UNKNOWN";
	}
}

static void dhcps_trace_packet(const char *stage, uint8_t state)
{
	if ((dhcp_message_repository == NULL) || (dhcps_netif == NULL)) {
		return;
	}

	printf("[DHCPS] %s state=%d(%s) if=%c%c xid=%02x%02x%02x%02x mac=%02x:%02x:%02x:%02x:%02x:%02x srv=%d.%d.%d.%d yi=%d.%d.%d.%d\r\n",
		stage,
		state,
		dhcps_state_string(state),
		dhcps_netif->name[0],
		dhcps_netif->name[1],
		dhcp_message_repository->xid[0],
		dhcp_message_repository->xid[1],
		dhcp_message_repository->xid[2],
		dhcp_message_repository->xid[3],
		dhcp_message_repository->chaddr[0],
		dhcp_message_repository->chaddr[1],
		dhcp_message_repository->chaddr[2],
		dhcp_message_repository->chaddr[3],
		dhcp_message_repository->chaddr[4],
		dhcp_message_repository->chaddr[5],
		ip4_addr1(&dhcps_local_address),
		ip4_addr2(&dhcps_local_address),
		ip4_addr3(&dhcps_local_address),
		ip4_addr4(&dhcps_local_address),
		ip4_addr1(&dhcps_allocated_client_address),
		ip4_addr2(&dhcps_allocated_client_address),
		ip4_addr3(&dhcps_allocated_client_address),
		ip4_addr4(&dhcps_allocated_client_address));
}
#endif
/**
  * @brief  latch the specific ip in the ip table.
  * @param  d the specific index
  * @retval None.
  */
#if (!IS_USE_FIXED_IP)
static void mark_ip_in_table(uint8_t d)
{
#if (debug_dhcps)
  	printf("\r\n mark ip %d\r\n",d);
#endif
	xSemaphoreTake(dhcps_ip_table_semaphore, portMAX_DELAY);
	if (0 < d && d <= 32) {
		ip_table.ip_range[0] = MARK_RANGE1_IP_BIT(ip_table, d);
#if (debug_dhcps)
		printf("\r\n ip_table.ip_range[0] = 0x%x\r\n",ip_table.ip_range[0]);
#endif
	} else if (32 < d && d <= 64) {
	  	ip_table.ip_range[1] = MARK_RANGE2_IP_BIT(ip_table, (d - 32));
#if (debug_dhcps)
		printf("\r\n ip_table.ip_range[1] = 0x%x\r\n",ip_table.ip_range[1]);
#endif
	} else if (64 < d && d <= 96) {
		ip_table.ip_range[2] = MARK_RANGE3_IP_BIT(ip_table, (d - 64));
#if (debug_dhcps)
		printf("\r\n ip_table.ip_range[2] = 0x%x\r\n",ip_table.ip_range[2]);
#endif
	} else if (96 < d && d <= 128) {
		ip_table.ip_range[3] = MARK_RANGE4_IP_BIT(ip_table, (d - 96));
#if (debug_dhcps)
		printf("\r\n ip_table.ip_range[3] = 0x%x\r\n",ip_table.ip_range[3]);
#endif
	} else {
	  	printf("\r\n Request ip over the range(1-128) \r\n");
	}
	xSemaphoreGive(dhcps_ip_table_semaphore);

}
#endif

/**
  * @brief  get one usable ip from the ip table of dhcp server.
  * @param: None
  * @retval the usable index which represent the ip4_addr(ip) of allocated ip addr.
  */
#if (!IS_USE_FIXED_IP)
static uint8_t search_next_ip(void)
{
	uint8_t range_count, offset_count;
	uint8_t start, end;
	if(dhcps_addr_pool_set){
		start = (uint8_t)ip4_addr4(&dhcps_addr_pool_start);
		end = (uint8_t)ip4_addr4(&dhcps_addr_pool_end);
	}else{
		start = 0;
		end = 255;
	}
	xSemaphoreTake(dhcps_ip_table_semaphore, portMAX_DELAY);
	for (range_count = 0; range_count < 4; range_count++) {
		for (offset_count = 0;offset_count < 32; offset_count++) {
			if ((((ip_table.ip_range[range_count] >> offset_count) & 0x01) == 0)
				&&(((range_count * 32) + (offset_count + 1)) >= start)
				&&(((range_count * 32) + (offset_count + 1)) <= end)) {
				xSemaphoreGive(dhcps_ip_table_semaphore);
				return ((range_count * 32) + (offset_count + 1));
			}
		}
	}
	xSemaphoreGive(dhcps_ip_table_semaphore);
	return 0;
}

static uint8_t is_ip_use(uint8_t ip)
{
    uint8_t range = (ip - 1) / 32;
    uint8_t offset = (ip - 1) % 32;

    return ((ip_table.ip_range[range] >> offset) & 0x01);
}
#endif

/**
  * @brief  fill in the option field with message type of a dhcp message.
  * @param  msg_option_base_addr: the addr be filled start.
  *	    message_type: the type code you want to fill in
  * @retval the start addr of the next dhcp option.
  */
static uint8_t *add_msg_type(uint8_t *msg_option_base_addr, uint8_t message_type)
{
	uint8_t *option_start;
	msg_option_base_addr[0] = DHCP_OPTION_CODE_MSG_TYPE;
	msg_option_base_addr[1] = DHCP_OPTION_LENGTH_ONE;
	msg_option_base_addr[2] = message_type;
	option_start = msg_option_base_addr + 3;
	if (DHCP_MESSAGE_TYPE_NAK == message_type)
		*option_start++ = DHCP_OPTION_CODE_END;
	return option_start;
}


static uint8_t *fill_one_option_content(uint8_t *option_base_addr,
	uint8_t option_code, uint8_t option_length, void *copy_info)
{
	uint8_t *option_data_base_address;
	uint8_t *next_option_start_address = NULL;
	option_base_addr[0] = option_code;
	option_base_addr[1] = option_length;
	option_data_base_address = option_base_addr + 2;
	switch (option_length) {
	case DHCP_OPTION_LENGTH_FOUR:
		memcpy(option_data_base_address, copy_info, DHCP_OPTION_LENGTH_FOUR);
		next_option_start_address = option_data_base_address + 4;
		break;
	case DHCP_OPTION_LENGTH_TWO:
		memcpy(option_data_base_address, copy_info, DHCP_OPTION_LENGTH_TWO);
		next_option_start_address = option_data_base_address + 2;
		break;
	case DHCP_OPTION_LENGTH_ONE:
		memcpy(option_data_base_address, copy_info, DHCP_OPTION_LENGTH_ONE);
		next_option_start_address = option_data_base_address + 1;
		break;
	}

	return next_option_start_address;
}

/**
  * @brief  fill in the needed content of the dhcp offer message.
  * @param  optptr  the addr which the tail of dhcp magic field.
  * @retval the addr represent to add the end of option.
  */
static void add_offer_options(uint8_t *option_start_address)
{
    uint8_t *temp_option_addr;
    /* add DHCP options 1.
    The subnet mask option specifies the client's subnet mask */
    temp_option_addr = fill_one_option_content(option_start_address,
        DHCP_OPTION_CODE_SUBNET_MASK, DHCP_OPTION_LENGTH_FOUR,
        (void *)&dhcps_local_mask);
#if 0
        /* add DHCP options 3 (i.e router(gateway)). The time server option
        specifies a list of RFC 868 [6] time servers available to the client. */
	temp_option_addr = fill_one_option_content(temp_option_addr,
			DHCP_OPTION_CODE_ROUTER, DHCP_OPTION_LENGTH_FOUR,
					(void *)&dhcps_local_address);

	/* add DHCP options 6 (i.e DNS).
        The option specifies a list of DNS servers available to the client. */
	temp_option_addr = fill_one_option_content(temp_option_addr,
			DHCP_OPTION_CODE_DNS_SERVER, DHCP_OPTION_LENGTH_FOUR,
					(void *)&dhcps_local_address);
#endif
    /* add DHCP options 51.
    This option is used to request a lease time for the IP address. */
    temp_option_addr = fill_one_option_content(temp_option_addr,
        DHCP_OPTION_CODE_LEASE_TIME, DHCP_OPTION_LENGTH_FOUR,
        (void *)&dhcp_option_lease_time_one_day);
    /* add DHCP options 54.
    The identifier is the IP address of the selected server. */
    temp_option_addr = fill_one_option_content(temp_option_addr,
        DHCP_OPTION_CODE_SERVER_ID, DHCP_OPTION_LENGTH_FOUR,
        (void *)&dhcps_local_address);
    /* add DHCP options 28.
    This option specifies the broadcast address in use on client's subnet.*/
    temp_option_addr = fill_one_option_content(temp_option_addr,
        DHCP_OPTION_CODE_BROADCAST_ADDRESS, DHCP_OPTION_LENGTH_FOUR,
        (void *)&dhcps_subnet_broadcast);
    /* add DHCP options 26.
    This option specifies the Maximum transmission unit to use */
    temp_option_addr = fill_one_option_content(temp_option_addr,
        DHCP_OPTION_CODE_INTERFACE_MTU, DHCP_OPTION_LENGTH_TWO,
        (void *)&dhcp_option_interface_mtu_576);
    /* add DHCP options 31.
    This option specifies whether or not the client should solicit routers */
    temp_option_addr = fill_one_option_content(temp_option_addr,
        DHCP_OPTION_CODE_PERFORM_ROUTER_DISCOVERY, DHCP_OPTION_LENGTH_ONE,
        (void *)&dhcp_option_router_discovery);
    *temp_option_addr++ = DHCP_OPTION_CODE_END;

}


/**
  * @brief  fill in common content of a dhcp message.
  * @param  m the pointer which point to the dhcp message store in.
  * @retval None.
  */
static void dhcps_initialize_message(struct dhcp_msg *dhcp_message_repository)
{

        dhcp_message_repository->op = DHCP_MESSAGE_OP_REPLY;
        dhcp_message_repository->htype = DHCP_MESSAGE_HTYPE;
        dhcp_message_repository->hlen = DHCP_MESSAGE_HLEN;
        dhcp_message_repository->hops = 0;
        memcpy((char *)dhcp_recorded_xid, (char *) dhcp_message_repository->xid,
					sizeof(dhcp_message_repository->xid));
        dhcp_message_repository->secs = 0;
        dhcp_message_repository->flags = htons(BOOTP_BROADCAST);

	memcpy((char *)dhcp_message_repository->yiaddr,
			(char *)&dhcps_allocated_client_address,
				sizeof(dhcp_message_repository->yiaddr));

	memset((char *)dhcp_message_repository->ciaddr, 0,
					sizeof(dhcp_message_repository->ciaddr));
        memset((char *)dhcp_message_repository->siaddr, 0,
					sizeof(dhcp_message_repository->siaddr));
        memset((char *)dhcp_message_repository->giaddr, 0,
					sizeof(dhcp_message_repository->giaddr));
        memset((char *)dhcp_message_repository->sname,  0,
					sizeof(dhcp_message_repository->sname));
        memset((char *)dhcp_message_repository->file,   0,
					sizeof(dhcp_message_repository->file));
        memset((char *)dhcp_message_repository->options, 0,
					dhcp_message_total_options_lenth);
        memcpy((char *)dhcp_message_repository->options, (char *)dhcp_magic_cookie,
					sizeof(dhcp_magic_cookie));
}

/**
  * @brief  init and fill in  the needed content of dhcp offer message.
  * @param  packet_buffer packet buffer for UDP.
  * @retval None.
  */
static void dhcps_send_offer(struct pbuf *packet_buffer)
{
	uint8_t temp_ip = 0;
	dhcp_message_repository = (struct dhcp_msg *)packet_buffer->payload;
#if (!IS_USE_FIXED_IP)
	/* create new client ip */
	temp_ip = search_next_ip();
#if (debug_dhcps)
	printf("\r\n temp_ip = %d",temp_ip);
#endif
	if (temp_ip == 0) {
#if 0
	  	memset(&ip_table, 0, sizeof(struct table));
		mark_ip_in_table((uint8_t)ip4_addr4(&dhcps_local_address));
		printf("\r\n reset ip table!!\r\n");
#endif
		printf("\r\n No useable ip!!!!\r\n");
	}

	IP4_ADDR(&dhcps_allocated_client_address, (ip4_addr1(&dhcps_network_id)),
		ip4_addr2(&dhcps_network_id), ip4_addr3(&dhcps_network_id), temp_ip);
#endif
        dhcps_initialize_message(dhcp_message_repository);
        add_offer_options(add_msg_type(&dhcp_message_repository->options[4],
						DHCP_MESSAGE_TYPE_OFFER));
        udp_sendto_if(dhcps_pcb, packet_buffer,
		   &dhcps_send_broadcast_address, DHCP_CLIENT_PORT, dhcps_netif);
}

/**
  * @brief  init and fill in  the needed content of dhcp nak message.
  * @param  packet buffer packet buffer for UDP.
  * @retval None.
  */
static void dhcps_send_nak(struct pbuf *packet_buffer)
{
	dhcp_message_repository = (struct dhcp_msg *)packet_buffer->payload;
        dhcps_initialize_message(dhcp_message_repository);
        add_msg_type(&dhcp_message_repository->options[4], DHCP_MESSAGE_TYPE_NAK);
        udp_sendto_if(dhcps_pcb, packet_buffer,
		   &dhcps_send_broadcast_address, DHCP_CLIENT_PORT, dhcps_netif);
}

/**
  * @brief  init and fill in  the needed content of dhcp ack message.
  * @param  packet buffer packet buffer for UDP.
  * @retval None.
  */
static void dhcps_send_ack(struct pbuf *packet_buffer)
{
        dhcp_message_repository = (struct dhcp_msg *)packet_buffer->payload;
        dhcps_initialize_message(dhcp_message_repository);
        add_offer_options(add_msg_type(&dhcp_message_repository->options[4],
				      			DHCP_MESSAGE_TYPE_ACK));
        udp_sendto_if(dhcps_pcb, packet_buffer,
		   &dhcps_send_broadcast_address, DHCP_CLIENT_PORT, dhcps_netif);
}

/**
  * @brief  according by the input message type to reflect the correspond state.
  * @param  option_message_type the input server state
  * @retval the server state which already transfer to.
  */
uint8_t dhcps_handle_state_machine_change(uint8_t option_message_type)
{
	switch (option_message_type) {
	case DHCP_MESSAGE_TYPE_DECLINE:
		dhcp_server_state_machine = DHCP_SERVER_STATE_IDLE;
		break;
	case DHCP_MESSAGE_TYPE_DISCOVER:
		if (dhcp_server_state_machine == DHCP_SERVER_STATE_IDLE) {
			dhcp_server_state_machine = DHCP_SERVER_STATE_OFFER;
		}
		break;
	case DHCP_MESSAGE_TYPE_REQUEST:

#if (!IS_USE_FIXED_IP)
		//DBG_PRINT("0x%x, 0x%x, 0x%x, 0x%x\r\n",ip_table.ip_range[0],ip_table.ip_range[1],ip_table.ip_range[2],ip_table.ip_range[3]);
		if (dhcp_server_state_machine == DHCP_SERVER_STATE_OFFER) {
			if (ip4_addr4(&dhcps_allocated_client_address) != 0) {
				if (memcmp((void *)&dhcps_allocated_client_address, (void *)&client_request_ip, 4) == 0) {
					dhcp_server_state_machine = DHCP_SERVER_STATE_ACK;
			  	} else {
				  	dhcp_server_state_machine = DHCP_SERVER_STATE_NAK;
			  	}
			} else {
			  	dhcp_server_state_machine = DHCP_SERVER_STATE_NAK;
			}
		} else {
			unsigned int *client_ip = (unsigned int*)(&dhcps_allocated_client_address);
			unsigned int *request_ip = (unsigned int*)(&client_request_ip);
			unsigned int default_domain = GP_AUTH_IP_ADDR0 + (GP_AUTH_IP_ADDR1<<8) + (GP_AUTH_IP_ADDR2<<16);
			unsigned int check_ip = *request_ip >> 24;			
			
			DBG_PRINT("fast mode\r\n");
			if ( (check_ip<DHCP_POOL_START)||(check_ip>DHCP_POOL_END) )
			{
				dhcp_server_state_machine = DHCP_SERVER_STATE_NAK;
				DBG_PRINT("IP over range\r\n");
			}
			else if ( (default_domain&0xFFFFFF)!=((*request_ip)&0xFFFFFF) )  // $$:$$:$$:xx
			{
				dhcp_server_state_machine = DHCP_SERVER_STATE_NAK;
				DBG_PRINT("Diff domain\r\n");
			}
			else if (g_ap_sta_num==1)
			{
				memcpy((void *)&dhcps_allocated_client_address, (void *)&client_request_ip, 4);
				dhcp_server_state_machine = DHCP_SERVER_STATE_ACK;
				DBG_PRINT("fast ok,(%x,%x)\r\n",*request_ip,*client_ip);
			}
            else if (is_ip_use(ip4_addr4(&client_request_ip)))
			{
				DBG_PRINT("0x%x is used\r\n",*client_ip);
				dhcp_server_state_machine = DHCP_SERVER_STATE_NAK;
            }
			else
			{
				DBG_PRINT("0x%x ack\r\n",*request_ip);
				memcpy((void *)&dhcps_allocated_client_address, (void *)&client_request_ip, 4);				
				dhcp_server_state_machine = DHCP_SERVER_STATE_ACK;
			}
		}
#else
		if (!(dhcp_server_state_machine == DHCP_SERVER_STATE_ACK ||
			dhcp_server_state_machine == DHCP_SERVER_STATE_NAK)) {
		        dhcp_server_state_machine = DHCP_SERVER_STATE_NAK;
		}
#endif
		break;
	case DHCP_MESSAGE_TYPE_RELEASE:
		dhcp_server_state_machine = DHCP_SERVER_STATE_IDLE;
		break;
	}

	return dhcp_server_state_machine;
}
/**
  * @brief  parse the dhcp message option part.
  * @param  optptr: the addr of the first option field.
  *         len: the total length of all option fields.
  * @retval dhcp server state.
  */
static uint8_t dhcps_handle_msg_options(uint8_t *option_start, int16_t total_option_length)
{

	int16_t option_message_type = 0;
        uint8_t *option_end = option_start + total_option_length;
        //dhcp_server_state_machine = DHCP_SERVER_STATE_IDLE;

        /* begin process the dhcp option info */
        while (option_start < option_end) {
                switch ((uint8_t)*option_start) {
                case DHCP_OPTION_CODE_MSG_TYPE:
                        option_message_type = *(option_start + 2); // 2 => code(1)+lenth(1)
                        break;
                case DHCP_OPTION_CODE_REQUEST_IP_ADDRESS :
#if IS_USE_FIXED_IP
			if (memcmp((char *)&dhcps_allocated_client_address,
						(char *)option_start + 2, 4) == 0)
			  	dhcp_server_state_machine = DHCP_SERVER_STATE_ACK;
			else
				dhcp_server_state_machine = DHCP_SERVER_STATE_NAK;
#else
			memcpy((char *)&client_request_ip, (char *)option_start + 2, 4);
#endif
                      	break;
                }
                // calculate the options offset to get next option's base addr
                option_start += option_start[1] + 2; // optptr[1]: length value + (code(1)+ Len(1))
        }
	return dhcps_handle_state_machine_change(option_message_type);
}

/**
  * @brief  get message from buffer then check whether it is dhcp related or not.
  *         if yes , parse it more to undersatnd the client's request.
  * @param  same as recv callback function definition
  * @retval if message is dhcp related then return dhcp server state,
  *	    otherwise return 0
  */
static uint8_t dhcps_check_msg_and_handle_options(struct pbuf *packet_buffer)
{
	int dhcp_message_option_offset;
        dhcp_message_repository = (struct dhcp_msg *)packet_buffer->payload;
	dhcp_message_option_offset = ((int)dhcp_message_repository->options
						- (int)packet_buffer->payload);
	dhcp_message_total_options_lenth = (packet_buffer->len
						- dhcp_message_option_offset);
	/* check the magic number,if correct parse the content of options */
	if (memcmp((char *)dhcp_message_repository->options,
		(char *)dhcp_magic_cookie, sizeof(dhcp_magic_cookie)) == 0) {
            	return dhcps_handle_msg_options(&dhcp_message_repository->options[4],
            				(dhcp_message_total_options_lenth - 4));
	}

	return 0;
}


/**
  * @brief  handle imcoming dhcp message and response message to client
  * @param  same as recv callback function definition
  * @retval None
  */
static void dhcps_receive_udp_packet_handler(void *arg, struct udp_pcb *udp_pcb,
struct pbuf *udp_packet_buffer, const ip_addr_t *sender_addr, u16_t sender_port)
{
  	int16_t total_length_of_packet_buffer;
	uint8_t dhcps_state;
	struct pbuf *merged_packet_buffer = NULL;

	if (udp_packet_buffer == NULL) {
		printf("\n\r Error!!!! System doesn't allocate any buffer \n\r");
		return;
	}
	dhcp_message_repository = (struct dhcp_msg *)udp_packet_buffer->payload;
	if (sender_port == DHCP_CLIENT_PORT) {
		total_length_of_packet_buffer = udp_packet_buffer->tot_len;
		if (udp_packet_buffer->next != NULL) {
			merged_packet_buffer = pbuf_coalesce(udp_packet_buffer,
								PBUF_TRANSPORT);
			if (merged_packet_buffer->tot_len !=
						total_length_of_packet_buffer) {
				pbuf_free(udp_packet_buffer);
				return;
			}
		}
		dhcps_state = dhcps_check_msg_and_handle_options(udp_packet_buffer);
#if ATBM_DHCPS_TRACE
		dhcps_trace_packet("rx", dhcps_state);
#endif
		switch (dhcps_state) {
		case  DHCP_SERVER_STATE_OFFER:
			dhcps_send_offer(udp_packet_buffer);
#if ATBM_DHCPS_TRACE
			dhcps_trace_packet("tx_offer", dhcps_state);
#endif
			break;
		case DHCP_SERVER_STATE_ACK:
			dhcps_send_ack(udp_packet_buffer);
#if ATBM_DHCPS_TRACE
			dhcps_trace_packet("tx_ack", dhcps_state);
#endif
#if (!IS_USE_FIXED_IP)
			mark_ip_in_table((uint8_t)ip4_addr4(&dhcps_allocated_client_address));
#endif
			dhcp_server_state_machine = DHCP_SERVER_STATE_IDLE;
			break;
		case DHCP_SERVER_STATE_NAK:
			dhcps_send_nak(udp_packet_buffer);
#if ATBM_DHCPS_TRACE
			dhcps_trace_packet("tx_nak", dhcps_state);
#endif
			dhcp_server_state_machine = DHCP_SERVER_STATE_IDLE;
			break;
		case DHCP_OPTION_CODE_END:
			break;
		}
        }

	/* free the UDP connection, so we can accept new clients */
        udp_disconnect(udp_pcb);

	/* Free the packet buffer */
	if (merged_packet_buffer != NULL)
		pbuf_free(merged_packet_buffer);
	else
		pbuf_free(udp_packet_buffer);
}

void dhcps_set_addr_pool(int addr_pool_set, ip_addr_t * addr_pool_start, ip_addr_t *addr_pool_end)
{
	if(addr_pool_set){
		dhcps_addr_pool_set = 1;

		memcpy(&dhcps_addr_pool_start, addr_pool_start,
							sizeof(ip_addr_t));

		memcpy(&dhcps_addr_pool_end, addr_pool_end,
							sizeof(ip_addr_t));
	}else{
		dhcps_addr_pool_set = 0;
	}
}
/**
  * @brief  Initialize dhcp server.
  * @param  None.
  * @retval None.
  * Note, for now,we assume the server latch ip 192.168.1.1 and support dynamic
  *       or fixed IP allocation.
  */
static ip_addr_t dhcps_pool_start;
static ip_addr_t dhcps_pool_end;



void dhcps_init(struct netif * pnetif)
{
//	printf("dhcps_init,wlan:%c\n\r",pnetif->name[1]);
	uint8_t *ip;

	dhcps_netif = pnetif;

	if (dhcps_pcb != NULL) {
		udp_remove(dhcps_pcb);
		dhcps_pcb = NULL;
	}

        dhcps_pcb = udp_new();
        IP4_ADDR(&dhcps_send_broadcast_address, 255, 255, 255, 255);
	/* get net info from net interface */

        memcpy(&dhcps_local_address, &pnetif->ip_addr,
        						sizeof(ip_addr_t));
	memcpy(&dhcps_local_mask, &pnetif->netmask,
							sizeof(ip_addr_t));

	memcpy(&dhcps_local_gateway, &pnetif->gw,
							sizeof(ip_addr_t));

	/* calculate the usable network ip range */
	dhcps_network_id.addr = ((pnetif->ip_addr.addr) &
					(pnetif->netmask.addr));

	dhcps_subnet_broadcast.addr = ((dhcps_network_id.addr |
					~(pnetif->netmask.addr)));
#if 0
	dhcps_owned_first_ip.addr = htonl((ntohl(dhcps_network_id.addr) + 1));
	dhcps_owned_last_ip.addr = htonl(ntohl(dhcps_subnet_broadcast.addr) - 1);
	dhcps_num_of_available_ips = ((ntohl(dhcps_owned_last_ip.addr)
				- ntohl(dhcps_owned_first_ip.addr)) + 1);
#endif

#if IS_USE_FIXED_IP
        IP4_ADDR(&dhcps_allocated_client_address, ip4_addr1(&dhcps_local_address)
		, ip4_addr2(&dhcps_local_address), ip4_addr3(&dhcps_local_address),
					(ip4_addr4(&dhcps_local_address)) + 1 );
#else
	if (dhcps_ip_table_semaphore != NULL) {
		vSemaphoreDelete(dhcps_ip_table_semaphore);
		dhcps_ip_table_semaphore = NULL;
	}
	dhcps_ip_table_semaphore = xSemaphoreCreateMutex();

	//dhcps_ip_table = (struct ip_table *)(pvPortMalloc(sizeof(struct ip_table)));
	memset(&ip_table, 0, sizeof(struct table));
	mark_ip_in_table((uint8_t)ip4_addr4(&dhcps_local_address));
	mark_ip_in_table((uint8_t)ip4_addr4(&dhcps_local_gateway));
#if 0
	for (i = 1; i < ip4_addr4(&dhcps_local_address); i++) {
		mark_ip_in_table(i);
	}
#endif
#endif

#if DHCP_POOL_START && DHCP_POOL_END
	memcpy(&dhcps_pool_start,&dhcps_local_address,sizeof(ip_addr_t));
	ip = (uint8_t *)&dhcps_pool_start;
	ip[3] = DHCP_POOL_START;
	memcpy(&dhcps_pool_end,&dhcps_local_address,sizeof(ip_addr_t));
	ip = (uint8_t *)&dhcps_pool_end;
	ip[3] = DHCP_POOL_END;

	dhcps_set_addr_pool(1,&dhcps_pool_start,&dhcps_pool_end);
#endif

    udp_bind(dhcps_pcb, IP_ADDR_ANY, DHCP_SERVER_PORT);
    udp_recv(dhcps_pcb, dhcps_receive_udp_packet_handler, NULL);
#if ATBM_DHCPS_TRACE
	printf("[DHCPS] init if=%c%c ip=%d.%d.%d.%d pool=%d-%d\r\n",
		pnetif->name[0],
		pnetif->name[1],
		ip4_addr1(&dhcps_local_address),
		ip4_addr2(&dhcps_local_address),
		ip4_addr3(&dhcps_local_address),
		ip4_addr4(&dhcps_local_address),
		DHCP_POOL_START,
		DHCP_POOL_END);
#endif
}

void dhcps_deinit(void)
{
	if (dhcps_pcb != NULL) {
		udp_remove(dhcps_pcb);
		dhcps_pcb = NULL;
	}
	if (dhcps_ip_table_semaphore != NULL) {
		vSemaphoreDelete(dhcps_ip_table_semaphore);
		dhcps_ip_table_semaphore = NULL;
	}
}

uint8_t lwip_search_next_ip(vlid)
{
	return search_next_ip();
}

