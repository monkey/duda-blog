/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef BLOG_POST_H
#define BLOG_POST_H

struct post {
    time_t date_ts;
    char  *date_hr;
    char  *id;
    char  *header;
};

char *home;

time_t post_get_timestamp(char *path);
char *post_format_ts(time_t ts);

#endif
