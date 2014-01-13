/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/* system */
#include <sys/types.h>
#include <dirent.h>

/* duda */
#include "webservice.h"
#include "packages/kv/kv.h"

/* service */
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
 * We use a Key/Value store to have a map between POSTs requests 
 * and the absolute path for each file representing the information.
 */
static char *blog_kv_post_get(char *hash)
{
    int rc;
    unqlite_int64 len;
    char *p = NULL;

    p = monkey->mem_alloc(MAX_PATH);
    len = MAX_PATH;
    rc = kv->fetch(posts_map, hash, -1, p, &len);
    if (rc != UNQLITE_OK) {
        monkey->mem_free(p);
        p = NULL;
    }

    return p;
}

/*
 * For a given request URI, compose the absolute path
 * of the Post.
 */
static char *blog_post_get_path(duda_request_t *dr)
{
    int pos;
    int offset;
    int title_len;
    int sec_len = sizeof(BLOG_POSTS_URI) - 1;
    char *id;
    char *date;
    char *title;
    char *path = NULL;
    char *dir_path = NULL;
    unsigned long len;
    DIR *d;
    struct dirent *dirent;

    pos = monkey->str_search_n(dr->sr->uri_processed.data,
                               BLOG_POSTS_URI,
                               MK_STR_SENSITIVE,
                               dr->sr->uri_processed.len);

    if (pos < 0 || dr->sr->uri_processed.len <= sec_len ||
        (dr->sr->uri_processed.len - (pos + sec_len)) < 12) {
        return NULL;
    }

    /* get the full POST id, e.g: '2014/01/12/Hello-World' */
    id = monkey->str_copy_substr(dr->sr->uri_processed.data,
                                 pos + sec_len,
                                 dr->sr->uri_processed.len);
    /*
     * once we get the ID, we perform a search in our Key Value store
     * to determinate whats the absolute path for the post file. If its
     * not found we perform a directory search.
     */
    path = blog_kv_post_get(id);
    if (path) {
        if (access(path, R_OK) != 0) {
            /* Invalidate the cache entry */
            kv->delete(posts_map, path, -1);
            monkey->mem_free(path);
            path = NULL;
        }
        else {
            return path;
        }
    }

    if (!path) {
        /*
         * the path was not found in our KV, lets open the directory that
         * contains blog posts for the given date and perform a search.
         */

        /* get POST date, eg: '2014/01/12' */
        offset = pos + sec_len;
        date = monkey->str_copy_substr(dr->sr->uri_processed.data,
                                       offset,
                                       offset + 10);
        if (!date) {
            monkey->mem_free(id);
            return NULL;
        }

        /* Compose the target directory path */
        monkey->str_build(&dir_path, &len,
                          "%s/posts/%s/",
                          data->get_path(),
                          date);

        d = opendir(dir_path);
        if (!d) {
            monkey->mem_free(id);
            monkey->mem_free(dir_path);
            return NULL;
        }

        /* get the POST title (previous ID + 11 chars offset) */
        title = id + 11;
        title_len = strlen(title);

        while ((dirent = readdir(d)) != NULL) {
            /*
             * A blog post file should have the following format on its name:
             *
             * hours-min-seconds_title.md
             *
             * where hours, minutes and seconds are two characters each one,
             * followed by an underscore and the Post Title, e.g:
             *
             *   18-40-20_Hello-World.md
             *
             * we are just interested into the title so we can skip the time
             * fields, its unlikely someone would post two entries in the same
             * day with the same title.
             */

            if (dirent->d_type != DT_REG) {
                continue;
            }

            if (dirent->d_name[0] == '.' || strcmp(dirent->d_name, "..") == 0) {
                continue;
            }

            if (strncmp(dirent->d_name + 9, title, title_len) == 0 &&
                strcmp(dirent->d_name + 9 + title_len, ".md") == 0) {
                monkey->str_build(&path, &len,
                                  "%s%s",
                                  dir_path,
                                  dirent->d_name);

                /* we have the path, now register this into the KV */
                kv->store(posts_map, id, -1, path, len);
                break;
            }
        }
        closedir(d);
    }

    monkey->mem_free(id);
    if (dir_path) {
        monkey->mem_free(dir_path);
    }

    if (access(path, R_OK) != 0) {
        monkey->mem_free(path);
        return NULL;
    }

    return path;
}

static char *blog_page_get_path(duda_request_t *dr)
{
    int pos;
    int sec_len = sizeof(BLOG_PAGES_URI) - 1;
    char *id;
    char *path = NULL;
    unsigned long len;

    pos = monkey->str_search_n(dr->sr->uri_processed.data,
                               BLOG_PAGES_URI,
                               MK_STR_SENSITIVE,
                               dr->sr->uri_processed.len);

    if (pos < 0 || dr->sr->uri_processed.len <= sec_len) {
        return NULL;
    }

    /* get the full POST id, e.g: '2014/01/12/Hello-World' */
    id = monkey->str_copy_substr(dr->sr->uri_processed.data,
                                 pos + sec_len,
                                 dr->sr->uri_processed.len);
    if (!id) {
        return NULL;
    }

    monkey->str_build(&path, &len,
                      "%s/pages/%s.md",
                      data->get_path(),
                      id);
    monkey->mem_free(id);

    if (access(path, R_OK) != 0) {
        monkey->mem_free(path);
        return NULL;
    }

    return path;
}

/* Wrapper callback to return a content with template data */
void cb_wrapper(duda_request_t *dr, int resource)
{
    char *path = NULL;

    if (resource == BLOG_POST) {
        path = blog_post_get_path(dr);
    }
    else if (resource == BLOG_PAGE) {
        path = blog_page_get_path(dr);
    }

    response->http_content_type(dr, "html");
    response->sendfile(dr, tpl_header);

    if (!path) {
        response->http_status(dr, 404);
        response->sendfile(dr, tpl_error_404);
    }
    else {
        /* Everything is OK, lets compose the response */
        response->http_status(dr, 200);
        response->sendfile(dr, tpl_post_header);
        response->sendfile(dr, path);
        response->sendfile(dr, tpl_post_footer);
        gc->add(dr, path);
    }

    response->sendfile(dr, tpl_footer);
    response->end(dr, NULL);
}

/* Callback for /posts/ */
void cb_posts(duda_request_t *dr)
{
    return cb_wrapper(dr, BLOG_POST);
}

/* Callback for /pages/ */
void cb_pages(duda_request_t *dr)
{
    return cb_wrapper(dr, BLOG_PAGE);
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
    /* Init packages */
    duda_load_package(kv, "kv");

    kv->init(&posts_map);

    /* Initialize template paths */
    blog_init_templates();

    conf->service_root();

    /* callbacks */
    map->static_add("/posts", "cb_posts");
    map->static_add("/pages", "cb_pages");
    map->static_root("cb_home");

    return 0;
}
