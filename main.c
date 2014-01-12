/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "webservice.h"


#include "blog.h"

DUDA_REGISTER("Duda I/O - Blog", "Blog");

/* Initialize fixed templates paths */
static int blog_init_templates()
{
    const char *p;
    char *tmp;

    /* get the absolute path for web service 'data/' directory */
    p = data->get_path();
    if (!p) {
        return -1;
    }

    /* header */
    tmp = monkey->mem_alloc(MAX_PATH);
    snprintf(tmp, MAX_PATH, "%s/templates/header.html", p);
    tpl_header = tmp;

    /* home intro */
    tmp = monkey->mem_alloc(MAX_PATH);
    snprintf(tmp, MAX_PATH, "%s/templates/home_intro.html", p);
    tpl_home_intro = tmp;

    /* blog post header */
    tmp = monkey->mem_alloc(MAX_PATH);
    snprintf(tmp, MAX_PATH, "%s/templates/post_header.html", p);
    tpl_post_header = tmp;

    /* blog post footer */
    tmp = monkey->mem_alloc(MAX_PATH);
    snprintf(tmp, MAX_PATH, "%s/templates/post_footer.html", p);
    tpl_post_footer = tmp;

    /* error 404: not found */
    tmp = monkey->mem_alloc(MAX_PATH);
    snprintf(tmp, MAX_PATH, "%s/templates/error_404.html", p);
    tpl_error_404 = tmp;

    /* footer */
    tmp = monkey->mem_alloc(MAX_PATH);
    snprintf(tmp, MAX_PATH, "%s/templates/footer.html", p);
    tpl_footer = tmp;

    return 0;
}

/*
 * For a given request URI and a section name, compose the absolute path
 * of the requested resource.
 */
static char *blog_get_path(duda_request_t *dr, char *section)
{
    int pos;
    char *id;
    char *path = NULL;
    unsigned long len;

    pos = monkey->str_search_n(dr->sr->uri_processed.data,
                               section,
                               MK_STR_SENSITIVE,
                               dr->sr->uri_processed.len);

    if (pos < 0 || dr->sr->uri_processed.len <= 7) {
        return NULL;
    }

    /* get the POST id or title */
    id = monkey->str_copy_substr(dr->sr->uri_processed.data,
                                 pos + 7,
                                 dr->sr->uri_processed.len);
    if (!id) {
        return NULL;
    }

    /* Check if the post exists */
    monkey->str_build(&path, &len,
                      "%s%s%s.md",
                      data->get_path(),
                      section,
                      id);
    monkey->mem_free(id);
    gc->add(dr, path);

    if (access(path, R_OK) != 0) {
        monkey->mem_free(path);
        return NULL;
    }

    return path;
}

/* Wrapper callback to return a content with template data */
void cb_wrapper(duda_request_t *dr, char *section)
{
    char *post_path = NULL;

    post_path = blog_get_path(dr, section);

    response->http_content_type(dr, "html");
    response->sendfile(dr, tpl_header);

    if (!post_path) {
        response->http_status(dr, 404);
        response->sendfile(dr, tpl_error_404);
    }
    else {
        /* Everything is OK, lets compose the response */
        response->http_status(dr, 200);
        response->sendfile(dr, tpl_post_header);
        response->sendfile(dr, post_path);
        response->sendfile(dr, tpl_post_footer);
    }

    response->sendfile(dr, tpl_footer);
    response->end(dr, NULL);
}

/* Callback for /posts/ */
void cb_posts(duda_request_t *dr)
{
    return cb_wrapper(dr, "/posts/");
}

/* Callback for /pages/ */
void cb_pages(duda_request_t *dr)
{
    return cb_wrapper(dr, "/pages/");
}

/* Callback for home page */
void cb_home(duda_request_t *dr)
{
    response->http_status(dr, 200);
    response->http_content_type(dr, "html");
    response->sendfile(dr, tpl_header);
    response->sendfile(dr, tpl_home_intro);
    response->sendfile(dr, tpl_footer);
    response->end(dr, NULL);
}

int duda_main()
{
    /* Initialize template paths */
    blog_init_templates();

    conf->service_root();

    /* callbacks */
    map->static_add("/posts", "cb_posts");
    map->static_add("/pages", "cb_pages");
    map->static_root("cb_home");

    return 0;
}
