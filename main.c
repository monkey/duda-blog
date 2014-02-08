/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*  Duda I/O Blog
 *  -------------
 *  Copyright (C) 2014, Eduardo Silva P. <edsiper@gmail.com>.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* system */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>

/* duda */
#include "webservice.h"
#include "packages/kv/kv.h"
#include "packages/sqlite/sqlite.h"

/* service */
#include "db.h"
#include "blog.h"
#include "post.h"

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
    tmp = mem->alloc(MK_MAX_PATH);
    snprintf(tmp, MK_MAX_PATH, "%s/templates/header.html", p);
    tpl_header = tmp;

    /* home intro */
    tmp = mem->alloc(MK_MAX_PATH);
    snprintf(tmp, MK_MAX_PATH, "%s/templates/home_intro.html", p);
    tpl_home_intro = tmp;

    /* blog post header */
    tmp = mem->alloc(MK_MAX_PATH);
    snprintf(tmp, MK_MAX_PATH, "%s/templates/post_header.html", p);
    tpl_post_header = tmp;

    /* blog post footer */
    tmp = mem->alloc(MK_MAX_PATH);
    snprintf(tmp, MK_MAX_PATH, "%s/templates/post_footer.html", p);
    tpl_post_footer = tmp;

    /* error 404: not found */
    tmp = mem->alloc(MK_MAX_PATH);
    snprintf(tmp, MK_MAX_PATH, "%s/templates/error_404.html", p);
    tpl_error_404 = tmp;

    /* footer */
    tmp = mem->alloc(MK_MAX_PATH);
    snprintf(tmp, MK_MAX_PATH, "%s/templates/footer.html", p);
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

    p = mem->alloc(MK_MAX_PATH);
    len = MK_MAX_PATH;
    rc = kv->fetch(posts_map, hash, -1, p, &len);
    if (rc != UNQLITE_OK) {
        mem->free(p);
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
    int sec_len = sizeof(BLOG_POSTS_URI) - 1;
    char *id;
    char *path = NULL;
    unsigned long len;

    pos = monkey->str_search_n(dr->sr->uri_processed.data,
                               BLOG_POSTS_URI,
                               MK_STR_SENSITIVE,
                               dr->sr->uri_processed.len);

    if (pos < 0 || dr->sr->uri_processed.len <= sec_len) {
        return NULL;
    }

    /* get the full POST id, e.g: '/Hello-World' */
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
            mem->free(path);
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

        /* Compose the target directory path */
        monkey->str_build(&path, &len,
                          "%s/posts/%s.md",
                          data->get_path(),
                          id);

        if (access(path, R_OK) != 0) {
            mem->free(path);
            return NULL;
        }

        kv->store(posts_map, id, -1, path, len);
    }

    mem->free(id);
    if (access(path, R_OK) != 0) {
        mem->free(path);
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

    /* Get the POST id (title) */
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
    mem->free(id);

    if (access(path, R_OK) != 0) {
        mem->free(path);
        return NULL;
    }

    return path;
}

/* Send out a range of registered blog entries */
void cb_page(duda_request_t *dr)
{
    time_t creat;
    char *path;
    char *date;
    char *title;
    struct tm *tmz;
    sqlite_handle_t *handle = NULL;
    char *sql = "SELECT creation, slug FROM posts ORDER BY id desc";

    path = mem->alloc(MK_MAX_PATH);
    date = mem->alloc(80);
    tmz  = mem->alloc(sizeof(struct tm));

    gc->add(dr, path);
    gc->add(dr, date);
    gc->add(dr, tmz);

    sqlite->dump(db, sql, &handle);
    sqlite_foreach(handle) {
        creat = sqlite->get_int(handle, 0);
        title = (char *) sqlite->get_text(handle, 1);

        gmtime_r(&creat, tmz);

        strftime(date, 80, "%B %d, %Y", tmz);
        snprintf(path, MK_MAX_PATH,
                 "%s/posts/%s.md",
                 data->get_path(),
                 title);

        response->printf(dr,
                         "<br /><span class='label label-primary'>%s</span>",
                         date);
        response->printf(dr, "<div class='duda-post'>");
        response->sendfile(dr, path);
        response->printf(dr, "</div>");
    }
    sqlite->done(handle);

    response->http_status(dr, 200);
    response->end(dr, NULL);
}

/*
 * We take care to serve static content from a callback instead of
 * Duda core because our posts are mapped starting from the first URI slash
 */
void cb_static (duda_request_t *dr)
{
    char *path = mem->alloc_z(MK_MAX_PATH);

    /* what a nasty hack */
    snprintf(path,
             self->docroot.len + 1 + dr->sr->uri_processed.len - 8,
             "%s%s", self->docroot.data, dr->sr->uri_processed.data + 8);
    gc->add(dr, path);

    response->http_status(dr, 200);
    response->sendfile(dr, path);
    response->end(dr, NULL);
}


/* Wrapper callback to return a content with template data */
void cb_wrapper(duda_request_t *dr, int resource)
{
    char *path = NULL;

    if (resource == BLOG_POST) {
        path    = blog_post_get_path(dr);
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
    duda_load_package(sqlite, "sqlite");

    /* Initialize database */
    blog_db_init();

    kv->init(&posts_map);

    /* Initialize template paths */
    blog_init_templates();

    conf->service_root();

    /* callbacks */
    map->static_add("/page/", "cb_page");
    map->static_add("/static/", "cb_static");
    map->static_add("/favicon.ico", "cb_static");
    map->static_add("/", "cb_posts");
    map->static_root("cb_home");

    return 0;
}
