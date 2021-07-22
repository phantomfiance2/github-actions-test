//*********************************************************
//
//    Copyright (c) Microsoft. All rights reserved.
//    This code is licensed under the Microsoft Public License.
//    THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
//    ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
//    IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
//    PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

typedef struct
{
    ngx_int_t buf_size;
    ngx_str_t mirror;
} ngx_http_dmirror_loc_conf_t;

typedef struct
{
    ngx_int_t status;
    ngx_array_t *payload;
} ngx_http_dmirror_ctx_t;

static void ngx_http_dmirror_body_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_dmirror_handler_internal(ngx_http_request_t *r);
static char *ngx_http_dmirror(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_dmirror_init(ngx_conf_t *cf);
static void *ngx_http_dmirror_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_dmirror_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);
static ngx_int_t ngx_http_dmirror_body_filter(ngx_http_request_t *r, ngx_chain_t *in);
static ngx_str_t concatenate_strings(ngx_http_request_t *r, ngx_str_t first, ngx_str_t second);
static int set_request_body(ngx_http_request_t *r, ngx_str_t tmp, ngx_http_request_t *sr);
static ngx_str_t convert_from_int(ngx_http_request_t *r, ngx_uint_t integer);
static ngx_int_t ngx_http_dmirror_handler(ngx_http_request_t *r);
static ngx_str_t create_header_string(ngx_http_request_t *r);
static ngx_str_t concatenate_if_header_not_null(ngx_http_request_t *r, ngx_table_elt_t *header, ngx_str_t result);
static ngx_str_t convert_array_to_str(ngx_http_request_t *r, ngx_array_t *array);

static ngx_command_t ngx_http_dmirror_commands[] = {

    {ngx_string("dmirror"),
     NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_CONF_TAKE2,
     ngx_http_dmirror,
     NGX_HTTP_LOC_CONF_OFFSET,
     0,
     NULL},

    ngx_null_command};

static ngx_http_module_t ngx_http_dmirror_module_ctx = {
    NULL,                                    /* preconfiguration */
    ngx_http_dmirror_init,                   /* postconfiguration */
    NULL,                                    /* create main configuration */
    NULL,                                    /* init main configuration */
    NULL,                                    /* create server configuration */
    NULL,                                    /* merge server configuration */
    ngx_http_dmirror_create_loc_conf,        /* create location configuration */
    ngx_http_dmirror_merge_loc_conf,         /* merge location configuration */
};

ngx_module_t ngx_http_dmirror_module = {
    NGX_MODULE_V1,
    &ngx_http_dmirror_module_ctx, /* module context */
    ngx_http_dmirror_commands,    /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING};

static ngx_http_output_body_filter_pt ngx_http_next_body_filter;

static char *
ngx_http_dmirror(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_dmirror_loc_conf_t *mlcf = conf;
    ngx_str_t *value;
    value = cf->args->elts;
    mlcf->mirror = value[1];
    ngx_int_t tmp = ngx_atoi(value[2].data, value[2].len);
    mlcf->buf_size = tmp * 1024;
    return NGX_CONF_OK;
}

static ngx_int_t
ngx_http_dmirror_init(ngx_conf_t *cf)
{
    ngx_http_handler_pt *h;
    ngx_http_core_main_conf_t *cmcf;
    ngx_http_next_body_filter = ngx_http_top_body_filter;
    ngx_http_top_body_filter = ngx_http_dmirror_body_filter;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_PRECONTENT_PHASE].handlers);
    if (h == NULL)
    {
        return NGX_ERROR;
    }

    *h = ngx_http_dmirror_handler;

    return NGX_OK;
}

static ngx_int_t
ngx_http_dmirror_handler(ngx_http_request_t *r)
{
    ngx_int_t rc;
    ngx_http_dmirror_ctx_t *ctx;

    if (r != r->main)
    {
        return NGX_DECLINED;
    }

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "dmirror handler");
    ctx = ngx_http_get_module_ctx(r, ngx_http_dmirror_module);
    if (ctx)
    {
        return ctx->status;
    }

    ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_dmirror_ctx_t));
    if (ctx == NULL)
    {
        return NGX_ERROR;
    }

    ctx->status = NGX_DONE;

    ngx_http_set_ctx(r, ctx, ngx_http_dmirror_module);

    rc = ngx_http_read_client_request_body(r, ngx_http_dmirror_body_handler);
    if (rc >= NGX_HTTP_SPECIAL_RESPONSE)
    {
        return rc;
    }

    ngx_http_finalize_request(r, NGX_DONE);
    return NGX_DONE;
}

static void
ngx_http_dmirror_body_handler(ngx_http_request_t *r)
{
    ngx_http_dmirror_ctx_t *ctx;

    ctx = ngx_http_get_module_ctx(r, ngx_http_dmirror_module);

    ctx->status = ngx_http_dmirror_handler_internal(r);

    r->preserve_body = 1;

    r->write_event_handler = ngx_http_core_run_phases;
    ngx_http_core_run_phases(r);
}

static ngx_int_t
ngx_http_dmirror_handler_internal(ngx_http_request_t *r)
{
    ngx_http_request_t *sr;
    ngx_str_t footer;
    ngx_http_dmirror_ctx_t *ctx;
    ngx_http_dmirror_loc_conf_t *flcf;

    flcf = ngx_http_get_module_loc_conf(r, ngx_http_dmirror_module);
    footer = flcf->mirror;

    if (footer.len == 0)
    {
        return NGX_DECLINED;
    }

    ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_dmirror_ctx_t));
    if (ctx == NULL)
    {
        return NGX_ERROR;
    }

    // We are initiating the array with 1kb size. We will grow this to
    // the max size as we ingest data.
    ctx->payload = ngx_array_create(r->pool, 1024, sizeof(u_char));
    if (ctx->payload == NULL)
    {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    ngx_http_set_ctx(r, ctx, ngx_http_dmirror_module);

    if (ngx_http_subrequest(r, &footer, &r->args, &sr, NULL,
                            NGX_HTTP_SUBREQUEST_BACKGROUND) != NGX_OK)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "Request was not successfully created. Request Method: \"%s\"",
                      r->method_name.data);
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    sr->header_only = 1;
    sr->method = NGX_HTTP_POST;
    ngx_str_t method_name_post = ngx_string("POST");
    sr->method_name = method_name_post;

    if (!sr->request_body->bufs)
    {
        ngx_str_t payload = ngx_string("{NGINX_EMPTY_94c6c609-8f84-4e3d-a67a-9c6bff4cdce3}");
        if (set_request_body(r, payload, sr) == NGX_ERROR)
        {
            return NGX_ERROR;
        }
    }

    return NGX_DECLINED;
}

static ngx_str_t convert_from_int(ngx_http_request_t *r, ngx_uint_t integer)
{
    int max_length = 20;
    u_char *data = ngx_pcalloc(r->pool, sizeof(u_char) * max_length);
    for (int i = 0; i < max_length; i++)
    {
        *(data + i) = 0;
    }
    ngx_sprintf(data, "%d", integer);
    int length = 0;
    for (int i = 0; i < max_length; i++)
    {
        if (*(data + i) == 0)
        {
            length = i;
            break;
        }
    }
    ngx_str_t result = ngx_string(data);
    result.len = length;
    return result;
}

static ngx_int_t
ngx_http_dmirror_body_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
    ngx_uint_t last;
    ngx_chain_t *cl;
    ngx_http_dmirror_ctx_t *ctx;
    ngx_http_request_t *sr;
    ngx_http_dmirror_loc_conf_t *flcf;
    flcf = ngx_http_get_module_loc_conf(r, ngx_http_dmirror_module);
    ctx = ngx_http_get_module_ctx(r, ngx_http_dmirror_module);
    if (ctx == NULL)
    {
        return ngx_http_next_body_filter(r, in);
    }

    last = 0;
    ngx_str_t statusStr = convert_from_int(r, r->headers_out.status);
    int index = 0;
    for (cl = in; cl; cl = cl->next)
    {
        if (ngx_buf_in_memory(cl->buf)) //todo dhrub create a work item to handle file responses
        {
            // copy the data into our output buffer.
            for (u_char *i = (cl->buf->pos); i < (cl->buf->last); i++, index++)
            {
                if ((ngx_int_t)ctx->payload->nelts < flcf->buf_size)
                {
                    u_char *c = ngx_array_push(ctx->payload);
                    if (c == NULL)
                    {
                        return NGX_HTTP_INTERNAL_SERVER_ERROR;
                    }
                    *c = *i;
                }
            }
        }

        if (cl->buf->last_buf)
        {
            last = 1;
            u_char *c = ngx_array_push(ctx->payload);
            if (c == NULL)
            {
                return NGX_HTTP_INTERNAL_SERVER_ERROR;
            }
            *c = 0; //set the null termination
            break;
        }
    }

    if (!last)
    {
        // There are more chunks to process continuing with the next handler
        return ngx_http_next_body_filter(r, in);
    }

    //At this point we have encountered the last chunk.
    if (ngx_http_subrequest(r, &flcf->mirror, &r->args, &sr, NULL,
                            NGX_HTTP_SUBREQUEST_BACKGROUND) != NGX_OK)
    {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    // set the method to be always post.
    sr->method = NGX_HTTP_POST;
    ngx_str_t method_name_post = ngx_string("POST");
    sr->method_name = method_name_post;

    // this is where we generate the result.
    // this is very ugly since I cannot declare and use ngx_string in a method call.
    ngx_str_t header_string = create_header_string(r);
    ngx_str_t newLine = ngx_string("\n");
    ngx_str_t tmp = convert_array_to_str(r, ctx->payload);
    ngx_str_t dataString = ngx_string("Data:\n");
    ngx_str_t data = concatenate_strings(r, dataString, tmp);
    ngx_str_t statusHeading = ngx_string("X-Microprotector-Status:");
    ngx_str_t statusHeadingComplete = concatenate_strings(r, statusHeading, statusStr);
    ngx_str_t statusWithNewLine = concatenate_strings(r, statusHeadingComplete, newLine);
    ngx_str_t result = concatenate_strings(r, statusWithNewLine, data);
    result = concatenate_strings(r, header_string, result);
    if (set_request_body(r, result, sr) == NGX_ERROR)
    {
        return NGX_ERROR;
    }

    return ngx_http_next_body_filter(r, in);
}

// We expect the input array to contain the null termination char as well
static ngx_str_t convert_array_to_str(ngx_http_request_t *r, ngx_array_t *array)
{
    u_char *output = ngx_pcalloc(r->pool, sizeof(u_char) * array->nelts);
    u_char *tmp = array->elts;
    for (ngx_uint_t i = 0; i < array->nelts; i++)
    {
        *(output + i) = *(tmp + i);
    }
    ngx_str_t result = ngx_string(output);
    result.len = array->nelts - 1;
    return result;
}

static ngx_str_t create_header_string(ngx_http_request_t *r)
{
    ngx_list_part_t *part;
    ngx_table_elt_t *h;
    ngx_uint_t i;

    ngx_str_t result = ngx_string("");
    /*
    Get the first part of the list. There is usual only one part.
    */
    part = &r->headers_out.headers.part;
    h = part->elts;

    /*
    Headers list array may consist of more than one part,
    so loop through all of it
    */
    for (i = 0; /* void */; i++)
    {
        if (i >= part->nelts)
        {
            if (part->next == NULL)
            {
                /* The last part, search is done. */
                break;
            }

            part = part->next;
            h = part->elts;
            i = 0;
        }

        // Process the header
        result = concatenate_if_header_not_null(r, &h[i], result);
    }

    /*
    These are the other special headers that are not in the headers_out.headers.
    We have to look at them separately. There is one additional header called
    Content-Type which is not covered here, we will have to look at it separately.
    ngx_table_elt_t                  *server;
    ngx_table_elt_t                  *date;
    ngx_table_elt_t                  *content_length;
    ngx_table_elt_t                  *content_encoding;
    ngx_table_elt_t                  *location;
    ngx_table_elt_t                  *refresh;
    ngx_table_elt_t                  *last_modified;
    ngx_table_elt_t                  *content_range;
    ngx_table_elt_t                  *accept_ranges;
    ngx_table_elt_t                  *www_authenticate;
    ngx_table_elt_t                  *expires;
    ngx_table_elt_t                  *etag;
    */
    result = concatenate_if_header_not_null(r, r->headers_out.server, result);
    result = concatenate_if_header_not_null(r, r->headers_out.date, result);
    result = concatenate_if_header_not_null(r, r->headers_out.content_length, result);
    result = concatenate_if_header_not_null(r, r->headers_out.content_encoding, result);
    result = concatenate_if_header_not_null(r, r->headers_out.location, result);
    result = concatenate_if_header_not_null(r, r->headers_out.refresh, result);
    result = concatenate_if_header_not_null(r, r->headers_out.last_modified, result);
    result = concatenate_if_header_not_null(r, r->headers_out.content_range, result);
    result = concatenate_if_header_not_null(r, r->headers_out.accept_ranges, result);
    result = concatenate_if_header_not_null(r, r->headers_out.www_authenticate, result);
    result = concatenate_if_header_not_null(r, r->headers_out.expires, result);
    result = concatenate_if_header_not_null(r, r->headers_out.etag, result);

    // Look at the content-type header separately.
    if (r->headers_out.content_type.len > 0)
    {
        ngx_table_elt_t *contentType = ngx_pcalloc(r->pool, sizeof(ngx_table_elt_t));
        ngx_str_t contentTypeKey = ngx_string("Content-Type");
        contentType->key = contentTypeKey;
        contentType->value = r->headers_out.content_type;
        result = concatenate_if_header_not_null(r, contentType, result);
    }

    return result;
}

static ngx_str_t concatenate_if_header_not_null(ngx_http_request_t *r, ngx_table_elt_t *header, ngx_str_t result)
{
    if (header == NULL)
    {
        return result;
    }

    ngx_str_t newLine = ngx_string("\n");
    ngx_str_t colon = ngx_string(":");
    ngx_str_t key = header->key;
    ngx_str_t value = header->value;
    ngx_str_t combined = concatenate_strings(r, key, colon);
    combined = concatenate_strings(r, combined, value);
    result = concatenate_strings(r, result, combined);
    result = concatenate_strings(r, result, newLine);
    return result;
}

// TODO this is not a very efficient use of the pool since we are always creating a new
// memory that is the size of the old memories. This would create an arithmetic progression
// of memory creations as we concatenate more and more strings. A better approach is to use the
// array_t and store raw u_chars there, use the array_t as a string buffer, then in one shot convert
// back to a string when needed.
static ngx_str_t concatenate_strings(ngx_http_request_t *r, ngx_str_t first, ngx_str_t second)
{
    int finalLength = first.len + second.len + 1;
    int i = 0;
    u_char *copy = ngx_pcalloc(r->pool, sizeof(u_char) * (finalLength));
    for (u_char *index = first.data; index < first.data + first.len; index++, i++)
    {
        *(copy + i) = *index;
    }

    for (u_char *index = second.data; index < second.data + second.len; index++, i++)
    {
        *(copy + i) = *index;
    }
    copy[finalLength - 1] = 0;
    ngx_str_t result = ngx_string(copy);
    result.len = finalLength - 1;
    return result;
}

static int set_request_body(ngx_http_request_t *r, ngx_str_t payload, ngx_http_request_t *sr)
{
    ngx_buf_t *buf;
    buf = ngx_calloc_buf(r->pool);
    if (buf == NULL)
    {
        return NGX_ERROR;
    }

    int i = 0;
    u_char *copy = ngx_pcalloc(r->pool, sizeof(u_char) * (payload.len + 1));
    for (u_char *index = payload.data; index <= payload.data + payload.len; index++, i++)
    {
        *(copy + i) = *index;
    }

    sr->headers_in.content_length_n = payload.len;

    buf->pos = copy;
    buf->last = buf->pos + payload.len;
    buf->start = buf->pos;
    buf->end = buf->last;
    buf->last_buf = 1;
    buf->memory = 0;
    
    sr->request_body = ngx_pcalloc(r->pool, sizeof(ngx_http_request_body_t));
    sr->request_body->bufs = ngx_pcalloc(r->pool, sizeof(ngx_chain_t));
    sr->request_body->bufs->buf = buf;
    sr->request_body->bufs->buf->temporary = 1;
    sr->request_body->bufs->next = 0;
    sr->header_only = 1;

    return NGX_OK;
}

static void *
ngx_http_dmirror_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_dmirror_loc_conf_t *conf;

    conf = ngx_pcalloc(cf->pool,
                       sizeof(ngx_http_dmirror_loc_conf_t));
    if (conf == NULL)
    {
        return NULL;
    }
    
    //conf->mirror = { 0, NULL }; is set by the pcalloc
    conf->buf_size = NGX_CONF_UNSET;

    return conf;
}

static char *
ngx_http_dmirror_merge_loc_conf(ngx_conf_t *cf, void *parent,
                                         void *child)
{
    ngx_http_dmirror_loc_conf_t *prev = parent;
    ngx_http_dmirror_loc_conf_t *conf = child;
    ngx_conf_merge_str_value(conf->mirror, prev->mirror, "");
    ngx_conf_merge_value(conf->buf_size, prev->buf_size, 0);

    return NGX_CONF_OK;
}