# Duda I/O Blog

This is the Blogging system used on http://blog.duda.io, a C based web service that generate and expose rendered Markdown files in HTML format.

It do not aim to be a complete blogging tool such as Wordpress, just only the basics to cover Duda I/O project needs:

 * a made at home Blog
 * Entries that can look as documentation or technical articles more than a common Blog post.
 * Managed from the command line
 * Place another Duda I/O application.

## Build Requirements

On Ubuntu systems:

 * libsqlite3-dev
 * libsqlite3
 * python-sqlite

## Build and Run

```
$ git clone https://github.com/monkey/dudac
$ git clone https://github.com/monkey/duda-blog
$ cd dudac
$ ./dudac -s
$ ./dudac -f -w ../duda-blog/ -p 8080
```

Now try the following URL:

  [http://localhost:8080/]([http://localhost:8080/)

## Installing the first post

In the examples directory you can find the Hello World post example, the goal of this file is to instruct how to install this post into the blog system using the _bladmin_ tool:

```
$ tools/bladmin -i examples/Hello-World.md .
```

the __-i__ argument represents the Markdown file to be inserted, the last _dot_ tells the _bladmin_ tool that the blog root directory is the same path where you are positionated.

Once the command runs, it will place a copy of the _Hello-World.md_ file into data/posts and insert a record into the SQLite database _data/blog.db_ specifying the title, slug, creation time and id. The database entry is only a reference to generate feeds.

## Contact
Eduardo Silva P. <edsiper@gmail.com>
