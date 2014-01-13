#ifndef DUDA_BLOG_H
#define DUDA_BLOG_H

/* KV */
kv_conn_t *posts_map;

/* templates */
char *tpl_header;
char *tpl_home_intro;
char *tpl_post_header;
char *tpl_post_footer;
char *tpl_error_404;
char *tpl_footer;

/* macros */
#define BLOG_POST         0x0
#define BLOG_PAGE         0x1

#define BLOG_PAGES_URI    "/pages/"
#define BLOG_POSTS_URI    "/posts/"
#endif
