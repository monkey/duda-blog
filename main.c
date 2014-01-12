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

void cb_posts(duda_request_t *dr)
{
    int pos;
    char *id;
    char *post_path = NULL;
    unsigned long len;

    pos = monkey->str_search_n(dr->sr->uri_processed.data,
                               "/posts/",
                               MK_STR_SENSITIVE,
                               dr->sr->uri_processed.len);

    if (pos < 0 || dr->sr->uri_processed.len <= 7) {
        goto not_found;
    }

    /* get the POST id or title */
    id = monkey->str_copy_substr(dr->sr->uri_processed.data,
                                 pos + 7,
                                 dr->sr->uri_processed.len);
    if (!id) {
        goto not_found;
    }
    else {
        gc->add(dr, id);
    }

    /* Check if the post exists */
    monkey->str_build(&post_path, &len,
                      "%s/posts/%s.md",
                      data->get_path(),
                      id);
    gc->add(dr, post_path);

    if (access(post_path, R_OK) != 0) {
        goto not_found;
    }

    /* Everything is OK, lets compose the response */
    response->http_status(dr, 200);
    response->http_content_type(dr, "html");
    response->sendfile(dr, tpl_header);
    response->sendfile(dr, tpl_post_header);
    response->sendfile(dr, post_path);
    response->sendfile(dr, tpl_post_footer);
    response->sendfile(dr, tpl_footer);
    response->end(dr, NULL);

 not_found:
    response->http_status(dr, 404);
    response->http_content_type(dr, "html");
    response->sendfile(dr, tpl_header);
    response->sendfile(dr, tpl_error_404);
    response->sendfile(dr, tpl_footer);
    response->end(dr, NULL);
}

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
    map->static_root("cb_home");

    return 0;
}
