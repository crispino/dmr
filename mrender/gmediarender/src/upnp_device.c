/* upnp_device.c - Generic UPnP device handling
 *
 * Copyright (C) 2005-2007   Ivo Clarysse
 *
 * This file is part of GMediaRender.
 *
 * GMediaRender is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GMediaRender is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GMediaRender; if not, write to the Free Software 
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
 * MA 02110-1301, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

//#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "output_gstreamer.h"

#include <upnp/upnp.h>
#include <upnp/ithread.h>
#include <upnp/upnptools.h>

#include "logging.h"

#include "xmlescape.h"
#include "webserver.h"
#include "upnp.h"
#include "upnp_device.h"
#include "upnp_transport.h"

#include "debug.h"

UpnpDevice_Handle device_handle;

static struct device *upnp_device;

int
upnp_add_response(struct action_event *event, char *key, const char *value)
{
	int rc = 0;
	char *val = NULL;
	int result = -1;

	ENTER();

	if (event->status) {
		goto out;
	}

	val = strdup(value);
	if (val == NULL) {
		/* report memory failure */
		event->status = -1;
		event->request->ActionResult = NULL;
		event->request->ErrCode = UPNP_SOAP_E_ACTION_FAILED;
		strcpy(event->request->ErrStr, strerror(errno));
		goto out;
	}

	rc =
	    UpnpAddToActionResponse(&event->request->ActionResult,
				    event->request->ActionName,
				    event->service->type, key, val);
	if (rc != UPNP_E_SUCCESS) {
		/* report custom error */
		event->request->ActionResult = NULL;
		event->request->ErrCode = UPNP_SOAP_E_ACTION_FAILED;
		strcpy(event->request->ErrStr, UpnpGetErrorMessage(rc));
		goto out;
	}

	result = 0;
out:
	LEAVE();
	free(val);
	return result;
}

int upnp_append_variable(struct action_event *event,
			 int varnum, char *paramname)
{
	char *value;
	struct service *service = event->service;
	int retval = -1;

	ENTER();

	if (varnum >= service->variable_count) {
		upnp_set_error(event, UPNP_E_INTERNAL_ERROR,
			       "Internal Error - illegal variable number %d",
			       varnum);
		goto out;
	}

	ithread_mutex_lock(service->service_mutex);

	value = (char *) service->variable_values[varnum];
	if (value == NULL) {
		upnp_set_error(event, UPNP_E_INTERNAL_ERROR,
			       "Internal Error");
	} else {
		retval = upnp_add_response(event, paramname, value);
	}

	ithread_mutex_unlock(service->service_mutex);
out:
	LEAVE();
	return retval;
}

void
upnp_set_error(struct action_event *event, int error_code,
	       const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	event->status = -1;
	event->request->ActionResult = NULL;
	event->request->ErrCode = UPNP_SOAP_E_ACTION_FAILED;
	vsnprintf(event->request->ErrStr, sizeof(event->request->ErrStr),
		  format, ap);
	va_end(ap);
	debug_printf(MSG_ERROR, "%s: %s\n", __FUNCTION__, event->request->ErrStr);
}


char *upnp_get_string(struct action_event *event, const char *key)
{
	IXML_Node *node;

	node = (IXML_Node *) event->request->ActionRequest;
	if (node == NULL) {
		upnp_set_error(event, UPNP_SOAP_E_INVALID_ARGS,
			       "Invalid action request document");
		return NULL;
	}
	node = ixmlNode_getFirstChild(node);
	if (node == NULL) {
		upnp_set_error(event, UPNP_SOAP_E_INVALID_ARGS,
			       "Invalid action request document");
		return NULL;
	}
	node = ixmlNode_getFirstChild(node);

	for (; node != NULL; node = ixmlNode_getNextSibling(node)) {
		if (strcmp(ixmlNode_getNodeName(node), key) == 0) {
			node = ixmlNode_getFirstChild(node);
			if (node == NULL) {
				/* Are we sure empty arguments are reported like this? */
				return strdup("");
			}
			return strdup(ixmlNode_getNodeValue(node));
		}
	}

	upnp_set_error(event, UPNP_SOAP_E_INVALID_ARGS,
		       "Missing action request argument (%s)", key);
	return NULL;
}

static int handle_subscription_request(struct Upnp_Subscription_Request
                                              *sr_event)
{
	struct service *srv;
	int i;
	int eventVarCount = 0, eventVarIdx = 0;
	const char **eventvar_names = NULL;
	char **eventvar_values = NULL;
	int rc;
	int result = -1;

	ENTER();

	srv = find_service(upnp_device, sr_event->ServiceId);
	if (srv == NULL) {
		debug_printf(MSG_ERROR, "%s: Unknown service '%s'\n", __FUNCTION__,
			sr_event->ServiceId);
		goto out;
	}

	ithread_mutex_lock(&(upnp_device->device_mutex));

	/* generate list of eventable variables */
	for(i=0; i<srv->variable_count; i++) {
		struct var_meta *metaEntry;
		metaEntry = &(srv->variable_meta[i]);
		if (metaEntry->sendevents == SENDEVENT_YES) {
			eventVarCount++;
		}
	}
	eventvar_names = malloc((eventVarCount+1) * sizeof(const char *));
	if (NULL == eventvar_names)
	{
	    debug_printf(MSG_ERROR, 
			"eventvar_names malloc fail, eventVarCount:%d\n", 
			eventVarCount);
		return 1;
	}
	eventvar_values = malloc((eventVarCount+1) * sizeof(const char *));
	if (NULL == eventvar_values)
	{
	    debug_printf(MSG_ERROR, 
			"eventvar_values malloc fail, eventVarCount:%d\n", 
			eventVarCount);
		free(eventvar_names);
		return 1;
	}
	//printf("%d evented variables\n", eventVarCount);

	for(i=0; i<srv->variable_count; i++) {
		struct var_meta *metaEntry;
		metaEntry = &(srv->variable_meta[i]);
		if (metaEntry->sendevents == SENDEVENT_YES) {
			eventvar_names[eventVarIdx] = srv->variable_names[i];
			eventvar_values[eventVarIdx] = xmlescape(srv->variable_values[i], 0);
			//printf("Evented: '%s' == '%s'\n",
			//	eventvar_names[eventVarIdx],
			//	eventvar_values[eventVarIdx]);
			eventVarIdx++;
		}
	}
	eventvar_names[eventVarIdx] = NULL;
	eventvar_values[eventVarIdx] = NULL;

	rc = UpnpAcceptSubscription(device_handle,
			       sr_event->UDN, sr_event->ServiceId,
			       (const char **)eventvar_names,
			       (const char **)eventvar_values,
			       eventVarCount,
			       sr_event->Sid);
	if (rc == UPNP_E_SUCCESS) {
		result = 0;
	}

	ithread_mutex_unlock(&(upnp_device->device_mutex));

	for(i=0; i<eventVarCount; i++) {
		free(eventvar_values[i]);
	}
	free(eventvar_names);
	free(eventvar_values);

out:
	LEAVE();
	return result;
}

static int handle_action_request(struct Upnp_Action_Request
					  *ar_event)
{
	struct service *event_service;
	struct action *event_action;

	event_service = find_service(upnp_device, ar_event->ServiceID);
	event_action = find_action(event_service, ar_event->ActionName);

	if (event_action == NULL) {
		debug_printf(MSG_ERROR, "Unknown action '%s' for service '%s'\n",
			ar_event->ActionName, ar_event->ServiceID);
		ar_event->ActionResult = NULL;
		ar_event->ErrCode = 401;
		return -1;
	}
	if (event_action->callback) {
		struct action_event event;
		int rc;
		event.request = ar_event;
		event.status = 0;
		event.service = event_service;

		rc = (event_action->callback) (&event);
		if (rc == 0) {
			ar_event->ErrCode = UPNP_E_SUCCESS;
			//printf("Action was a success!\n");
		}
		if (ar_event->ActionResult == NULL) {
			ar_event->ActionResult =
			    UpnpMakeActionResponse(ar_event->ActionName,
						   ar_event->ServiceID, 0,
						   NULL);
		}
	} else {
		debug_printf(MSG_ERROR, 
			"Got a valid action, but no handler defined (!)\n");
		debug_printf(MSG_ERROR, "  ErrCode:    %d\n", ar_event->ErrCode);
		debug_printf(MSG_ERROR, "  Socket:     %d\n", ar_event->Socket);
		debug_printf(MSG_ERROR, "  ErrStr:     '%s'\n", ar_event->ErrStr);
		debug_printf(MSG_ERROR, "  ActionName: '%s'\n",
			ar_event->ActionName);
		debug_printf(MSG_ERROR, "  DevUDN:     '%s'\n", ar_event->DevUDN);
		debug_printf(MSG_ERROR, "  ServiceID:  '%s'\n",
			ar_event->ServiceID);
		ar_event->ErrCode = UPNP_E_SUCCESS;
	}



	return 0;
}

static int event_handler(Upnp_EventType EventType, void *event,
			    void *Cookie)
{
	switch (EventType) {
	case UPNP_CONTROL_ACTION_REQUEST:
		handle_action_request(event);
		break;
	case UPNP_CONTROL_GET_VAR_REQUEST:
		debug_printf(MSG_INFO, "NOT IMPLEMENTED: control get variable request\n");
		break;
	case UPNP_EVENT_SUBSCRIPTION_REQUEST:
		//printf("event subscription request\n");
		handle_subscription_request(event);
		break;

	default:
		debug_printf(MSG_INFO, "Unknown event type: %d\n", EventType);
		break;
	}
	return 0;
}


int upnp_device_init(struct device *device_def, char *ip_address)
{
	int rc;
	int result = -1;
	short int port = 0;
	struct service *srv;
	struct icon *icon_entry;
	char *buf;
	int i;

	if (device_def->init_function)
	{
		rc = device_def->init_function();
		if (rc != 0) {
			goto out;
		}
	}

	upnp_device = device_def;

	/* register icons in web server */
        for (i=0; (icon_entry = upnp_device->icons[i]); i++) {
		webserver_register_file(icon_entry->url, "image/png");
        }

	/* generate and register service schemas in web server */
        for (i=0; (srv = upnp_device->services[i]); i++) {
       		buf = upnp_get_scpd(srv);
            debug_printf(MSG_INFO, "registering '%s'\n", srv->scpd_url);
		webserver_register_buf(srv->scpd_url,buf,"text/xml");
	}


	rc = UpnpInit(ip_address, port);
	if (UPNP_E_SUCCESS != rc) {
		debug_printf(MSG_ERROR, "UpnpInit() Error: %d\n", rc);
		goto upnp_err_out;
	}
	rc = UpnpEnableWebserver(TRUE);
	if (UPNP_E_SUCCESS != rc) {
		debug_printf(MSG_ERROR, "UpnpEnableWebServer() Error: %d\n", rc);
		goto upnp_err_out;
	}
	rc = UpnpSetVirtualDirCallbacks(&virtual_dir_callbacks);
	if (UPNP_E_SUCCESS != rc) {
		debug_printf(MSG_ERROR, "UpnpSetVirtualDirCallbacks() Error: %d\n", rc);
		goto upnp_err_out;
	}
	rc = UpnpAddVirtualDir("/upnp");
	if (UPNP_E_SUCCESS != rc) {
		debug_printf(MSG_ERROR, "UpnpAddVirtualDir() Error: %d\n", rc);
		goto upnp_err_out;
	}

       	buf = upnp_get_device_desc(device_def);

	rc = UpnpRegisterRootDevice2(UPNPREG_BUF_DESC,
				     buf, strlen(buf), 1,
				     &event_handler, &device_def,
				     &device_handle);
	if (UPNP_E_SUCCESS != rc) {
		debug_printf(MSG_ERROR, "UpnpRegisterRootDevice2() Error: %d\n", rc);
		goto upnp_err_out;
	}
	rc = UpnpSendAdvertisement(device_handle, 100);
	if (UPNP_E_SUCCESS != rc) {
		debug_printf(MSG_ERROR, "Error sending advertisements: %d\n", rc);
		goto upnp_err_out;
	}
	result = 0;
	goto out;

upnp_err_out:
	UpnpFinish();
out:
	return result;
}


