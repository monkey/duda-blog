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

#include "webservice.h"
#include "db.h"

static int blog_db_install(sqlite_db_t *db)
{
    sqlite_handle_t *handle;

    char *sql =
        "CREATE TABLE posts ("
        "id           INTEGER PRIMARY KEY ASC,"
        "creation     INTEGER,"
        "modification INTEGER,"
        "title        TEXT,"
        "slug         TEXT,"
        "summary      TEXT"
        ");";

    sqlite->dump(db, sql, &handle);
    sqlite->step(handle);
    return sqlite->done(handle);
}

/*
 * Initialize the SQLite database, make sure the DB exists on the file system,
 * otherwise create and put the schemas on place.
 */
int blog_db_init()
{
    int ret;
    int create = MK_FALSE;
    char db_path[MK_MAX_PATH];
    struct file_info f;

    snprintf(db_path, MK_MAX_PATH,
             "%s/%s",
             data->get_path(),
             BLOG_DB_NAME);

    ret = monkey->file_get_info(db_path, &f);
    if (ret != 0) {
        msg->warn("Blog: Database don't exists, it will be created");
        create = MK_TRUE;
    }

    db = sqlite->open(db_path);
    if (!db) {
        msg->err("Blog: Database could not be opened/created. Aborting");
        exit(EXIT_FAILURE);
    }

    msg->info("Blog: Database opened/created");

    /* If the database did not exists previously, load the schema */
    if (create == MK_TRUE) {
        blog_db_install(db);
    }

    return 0;
}
