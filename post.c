/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

#include "webservice.h"
#include "post.h"

/*
 * From a given Markdown file, extract the first line with the
 * timestamp.
 */
time_t post_get_timestamp(char *path)
{
    int fd;
    int bytes;
    char *date;
    time_t t;

    fd = open(path, O_RDONLY);
    if (fd == -1) {
        return 0;
    }

    date = monkey->mem_alloc(18);
    bytes = read(fd, date, 17);
    if (bytes <= 0) {
        monkey->mem_free(date);
        close(fd);
        return 0;
    }
    close(fd);

    t = strtol(date + 7, NULL, 10);
    monkey->mem_free(date);

    return t;
}

/*
 * From a given Markdown file, retrieve a converted date of
 * the post from timestamp to human readable format
 */
char *post_format_ts(time_t ts)
{
    char *buf;
    struct tm *gtm;

    gtm = monkey->mem_alloc(sizeof(struct tm));
    gtm = gmtime_r(&ts, gtm);
    if (!gtm) {
        return NULL;
    }

    /* Compose template */
    buf = monkey->mem_alloc_z(32);
    strftime(buf, 32, "###### %B %d, %Y", gtm);
    monkey->mem_free(gtm);

    return buf;
}

/* Open a post file and extract a header (skip timestamp) */
char *post_get_header(char *path)
{
    int i;
    int fd;
    int blanks = 0;
    int buf_size = 512/2;
    int bytes;
    char *buf;

    fd = open(path, O_RDONLY);
    if (fd == -1) {
        return NULL;
    }

    buf = monkey->mem_alloc(buf_size);
    if (!buf) {
        return NULL;
    }

    lseek(fd, 19, SEEK_SET);
    bytes = read(fd, buf, buf_size - 3);
    if (bytes <= 0) {
        monkey->mem_free(buf);
        return NULL;
    }

    for (i = bytes -1 ; buf[i] != ' '; i--) {
        buf[i] = ' ';
        blanks++;
    }

    buf[i++] = '.';
    buf[i++] = '.';
    buf[i++] = '.';

    return buf;
}

static int _generator_arr_cmp(const void *a, const void *b)
{
    struct post *pa = (struct post *) a;
    struct post *pb = (struct post *) b;

    if (pa->date_ts == pb->date_ts)
        return 0;
    else
        if (pa->date_ts < pb->date_ts)
            return 1;
        else
            return -1;
}

/*
 * This function do its job in a worker thread, it take
 * cares of refresh the POST lists and RSS feeds.
 */
int post_generator()
{
    DIR *d;
    int i;
    int len;
    int count = 0;
    int arr_size = 1024;
    time_t t;
    struct post **post_arr;
    struct post **temp_arr;
    char post_path[MAX_PATH];
    char posts[MAX_PATH];
    struct dirent *dirent;
    struct mk_list list;

    /* First step generate a sorted list of available posts */
    snprintf(posts, MAX_PATH, "%s/posts/", data->get_path());

    d = opendir(posts);
    if (!d) {
        msg->err("Cannot open data/posts/ directory");
        exit(EXIT_FAILURE);
    }

    mk_list_init(&list);

    count = 0;
    post_arr = monkey->mem_alloc(sizeof(struct post) * arr_size);

    /* Read the posts directory and create a list of entries */
    while ((dirent = readdir(d)) != NULL) {
        if (dirent->d_type != DT_REG) {
            continue;
        }

        if (dirent->d_name[0] == '.' || strcmp(dirent->d_name, "..") == 0) {
            continue;
        }

        len = strlen(dirent->d_name);

        if (strcmp(dirent->d_name + (len - 3), ".md") != 0) {
            continue;
        }

        snprintf(post_path, MAX_PATH, "%s/posts/%s",
                 data->get_path(), dirent->d_name);

        t = post_get_timestamp(post_path);
        if (t <= 0) {
            continue;
        }

        if (count >= arr_size) {
            arr_size += 1024;
            temp_arr = monkey->mem_realloc(post_arr, arr_size);
            if (temp_arr) {
                post_arr = temp_arr;
            }
        }

        post_arr[count] = monkey->mem_alloc(sizeof(struct post));
        post_arr[count]->date_ts = t;
        post_arr[count]->date_hr = post_format_ts(t);
        post_arr[count]->id      = monkey->str_dup(dirent->d_name);
        post_arr[count]->header  = post_get_header(post_path);
        count++;
    }
    closedir(d);


    /* Sort in reverse order */
    qsort(post_arr, count, sizeof(struct post), _generator_arr_cmp);

    for (i = 0; i < count; i++) {
        printf("%10lu %-24s %s\n",
               post_arr[i]->date_ts,
               post_arr[i]->date_hr,
               post_arr[i]->id);
    }

    return 0;
}
