# Writing a Post

---

Using the Duda Blogging system assumes that you are friendly with the Linux command line and knows what is Markdown.

## Filename

Each post file must have on it's name the SLUG without blank spaces in the middle, and end in _.md_, for e.g: if you want to write a post entry about _save the world_, the filename candidate would be _Save-the-world.md_ .

The file extension _.md_ is used to let the blog core identify a Post inside the file system where they are stored.

## Markdown

All file content is written in Markdown and the blogging system integrates the extension called [Github Flavored Markdown](http://github.github.com/github-flavored-markdown/).

## Install a Post

Even is not necesary to make a Post valid into the Database, this process helps to add the entry to the index so it can be listed in the home page as well be included in the RSS feed.

To install a Post review the command line program _bladmin_ located under the _tools/_ directory, to install a Post or update it do:

```Bash
$ tools/bladmin -i examples/Save-the-world.md .
```
