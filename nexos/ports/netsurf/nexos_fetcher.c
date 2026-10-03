/* NexOS NetSurf fetcher: the NetSurf fetcher ABI over existing http_get(). */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils/corestrings.h"
#include "utils/errors.h"
#include "utils/nsurl.h"
#include "netsurf/misc.h"
#include "content/fetch.h"
#include "content/fetchers.h"
#include "kernel/net/http.h"

struct nexos_fetch_info {
	struct fetch *fetch_handle;
	nsurl *url;
	bool only_2xx;
	bool completed;
};

static struct nexos_fetch_info *active_fetch;

static bool nexos_fetch_initialise(lwc_string *scheme)
{
	(void)scheme;
	return true;
}

static bool nexos_fetch_acceptable(const nsurl *url)
{
	(void)url;
	return true;
}

static void *nexos_fetch_setup(struct fetch *parent_fetch, nsurl *url,
				       bool only_2xx, bool downgrade_tls,
				       const char *post_urlenc,
				       const struct fetch_multipart_data *post_multipart,
				       const char **headers)
{
	struct nexos_fetch_info *fetch;
	(void)downgrade_tls;
	(void)post_urlenc;
	(void)post_multipart;
	(void)headers;

	/* The existing NexOS HTTP boundary is currently GET-only. */
	fetch = calloc(1, sizeof(*fetch));
	if (fetch == NULL) return NULL;
	fetch->fetch_handle = parent_fetch;
	fetch->url = nsurl_ref(url);
	fetch->only_2xx = only_2xx;
	return fetch;
}

static bool nexos_fetch_start(void *data)
{
	struct nexos_fetch_info *fetch = data;
	http_response_t *response;
	fetch_msg msg;
	char header[96];

	/* Serialize the first milestone over the kernel's existing single HTTP
	 * request boundary. Additional requests remain queued and are retried by
	 * the normal NetSurf fetch scheduler. */
	if (active_fetch != NULL) return false;
	active_fetch = fetch;
	response = http_get(nsurl_access(fetch->url));
	if (response == NULL) {
		msg.type = FETCH_ERROR;
		msg.data.error = "NexOS HTTP request failed";
		fetch_send_callback(&msg, fetch->fetch_handle);
		fetch->completed = true;
		return true;
	}

	fetch_set_http_code(fetch->fetch_handle, response->status_code);
	if (fetch->only_2xx && (response->status_code < 200 ||
				       response->status_code >= 300)) {
		msg.type = FETCH_ERROR;
		msg.data.error = "Not2xx";
		fetch_send_callback(&msg, fetch->fetch_handle);
	} else {
		int n = snprintf(header, sizeof(header),
				 "HTTP/1.1 %d\r\nContent-Type: text/html\r\n\r\n",
				 response->status_code);
		msg.type = FETCH_HEADER;
		msg.data.header_or_data.buf = (const uint8_t *)header;
		msg.data.header_or_data.len = (size_t)n;
		fetch_send_callback(&msg, fetch->fetch_handle);

		if (response->body_len != 0) {
			msg.type = FETCH_DATA;
			msg.data.header_or_data.buf = response->body;
			msg.data.header_or_data.len = response->body_len;
			fetch_send_callback(&msg, fetch->fetch_handle);
		}
		msg.type = FETCH_FINISHED;
		fetch_send_callback(&msg, fetch->fetch_handle);
	}

	http_free(response);
	fetch->completed = true;
	return true;
}

static void nexos_fetch_abort(void *data)
{
	struct nexos_fetch_info *fetch = data;
	/* http_get is synchronous; an in-flight request is allowed to finish. */
	if (fetch != NULL) fetch->completed = true;
}

static void nexos_fetch_free(void *data)
{
	struct nexos_fetch_info *fetch = data;
	if (fetch == NULL) return;
	nsurl_unref(fetch->url);
	free(fetch);
}

static void nexos_fetch_poll(lwc_string *scheme)
{
	struct nexos_fetch_info *fetch = active_fetch;
	(void)scheme;
	if (fetch == NULL || !fetch->completed) return;
	active_fetch = NULL;
	fetch_remove_from_queues(fetch->fetch_handle);
	fetch_free(fetch->fetch_handle);
}

static int nexos_fetch_fdset(lwc_string *scheme, fd_set *read_set,
				     fd_set *write_set, fd_set *error_set)
{
	(void)scheme;
	(void)read_set;
	(void)write_set;
	(void)error_set;
	return 0;
}

static void nexos_fetch_finalise(lwc_string *scheme)
{
	(void)scheme;
}

nserror fetch_nexos_register(void)
{
	static const struct fetcher_operation_table ops = {
		.initialise = nexos_fetch_initialise,
		.acceptable = nexos_fetch_acceptable,
		.setup = nexos_fetch_setup,
		.start = nexos_fetch_start,
		.abort = nexos_fetch_abort,
		.free = nexos_fetch_free,
		.poll = nexos_fetch_poll,
		.fdset = nexos_fetch_fdset,
		.finalise = nexos_fetch_finalise,
	};
	nserror error;

	error = fetcher_add(lwc_string_ref(corestring_lwc_http), &ops);
	if (error != NSERROR_OK) return error;
	return fetcher_add(lwc_string_ref(corestring_lwc_https), &ops);
}
