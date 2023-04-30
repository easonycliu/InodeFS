// chfs client.  implements FS operations using extent and lock server
#include "chfs_client.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

chfs_client::chfs_client()
{
    ec = new extent_client();

}

chfs_client::chfs_client(std::string extent_dst, std::string lock_dst)
{
    ec = new extent_client();
    std::cout << "Ini ChFS Client" << std::endl;
    if (ec->put(1, "") != extent_protocol::OK)
        printf("error init root dir\n"); // XYB: init root dir
}

chfs_client::inum
chfs_client::n2i(std::string n)
{
    std::istringstream ist(n);
    unsigned long long finum;
    ist >> finum;
    return finum;
}

std::string
chfs_client::filename(inum inum)
{
    std::ostringstream ost;
    ost << inum;
    return ost.str();
}

bool
chfs_client::isfile(inum inum)
{
    extent_protocol::attr a;

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        return false;
    }

    if (a.type == extent_protocol::T_FILE) {
        printf("isfile: %lld is a file\n", inum);
        return true;
    }
    return false;
}
/** Your code here for Lab...
 * You may need to add routines such as
 * readlink, issymlink here to implement symbolic link.
 * 
 * */

bool
chfs_client::isdir(inum inum)
{
    // Oops! is this still correct when you implement symlink?
    extent_protocol::attr a;

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        return false;
    }

    if (a.type == extent_protocol::T_DIR) {
        printf("isdir: %lld is a dir\n", inum);
        return true;
    }
    return false;
}

int
chfs_client::getfile(inum inum, fileinfo &fin)
{
    int r = OK;

    printf("getfile %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }

    fin.atime = a.atime;
    fin.mtime = a.mtime;
    fin.ctime = a.ctime;
    fin.size = a.size;
    printf("getfile %016llx -> sz %llu\n", inum, fin.size);

release:
    return r;
}

int
chfs_client::getdir(inum inum, dirinfo &din)
{
    int r = OK;

    printf("getdir %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }
    din.atime = a.atime;
    din.mtime = a.mtime;
    din.ctime = a.ctime;

release:
    return r;
}


#define EXT_RPC(xx) do { \
    if ((xx) != extent_protocol::OK) { \
        printf("EXT_RPC Error: %s:%d \n", __FILE__, __LINE__); \
        r = IOERR; \
        goto release; \
    } \
} while (0)

// Only support set size of attr
int
chfs_client::setattr(inum ino, size_t size)
{
    int r = OK;

    /*
     * your code goes here.
     * note: get the content of inode ino, and modify its content
     * according to the size (<, =, or >) content length.
     */
    r = ec->set_attr(ino, size);
    

    return r;
}

int
chfs_client::create(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    int r = OK;

    /*
     * your code goes here.
     * note: lookup is what you need to check if file exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */
    bool file_exist = false;
    inum lookup_ino = ino_out;
    lookup(parent, name, file_exist, lookup_ino);
    if (file_exist)
    {
        r = EXIST;
    }
    else
    {
        r = ec->create(extent_protocol::T_FILE, ino_out);
        ec->add_to_dir(parent, ino_out, std::string(name));
        std::cout << "chfs_client : create " << name << " with ino " << ino_out << std::endl;
    }
    return r;
}

int
chfs_client::mkdir(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    int r = OK;

    /*
     * your code goes here.
     * note: lookup is what you need to check if directory exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */
    bool dir_exist = false;
    inum lookup_ino = ino_out;
    lookup(parent, name, dir_exist, lookup_ino);
    if (dir_exist)
    {
        r = EXIST;
    }
    else
    {
        r = ec->create(extent_protocol::T_DIR, ino_out);
        ec->add_to_dir(parent, ino_out, std::string(name));
        std::cout << "chfs_client : create " << name << " with ino " << ino_out << std::endl;
    }
    return r;
}

int 
chfs_client::ln(inum parent, const char *name, mode_t mode, inum &ino_out)
{
int r = OK;

    /*
     * your code goes here.
     * note: lookup is what you need to check if directory exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */
    bool link_exist = false;
    inum lookup_ino = ino_out;
    lookup(parent, name, link_exist, lookup_ino);
    if (link_exist)
    {
        r = EXIST;
    }
    else
    {
        r = ec->create(extent_protocol::T_LNK, ino_out);
        ec->add_to_dir(parent, ino_out, std::string(name));
    }
    return r;
}

int
chfs_client::lookup(inum parent, const char *name, bool &found, inum &ino_out)
{
    int r = OK;

    /*
     * your code goes here.
     * note: lookup file from parent dir according to name;
     * you should design the format of directory content.
     */
    std::list<dirent> contents;
    readdir(parent, contents);
    for (auto content : contents)
    {
        // printf("chfs_client: find dir %s\n", content.name.c_str());
        if (std::string(name) == content.name)
        {
            found = true;
            ino_out = content.inum;
            break;
        }
    }

    return r;
}

int
chfs_client::readdir(inum dir, std::list<dirent> &list)
{
    int r = OK;

    /*
     * your code goes here.
     * note: you should parse the dirctory content using your defined format,
     * and push the dirents to the list.
     */
    std::vector<std::pair<inum, std::string>> bufs;
    ec->read_dir(dir, bufs);
    for (auto buf : bufs)
    {
        dirent content;
        content.inum = buf.first;
        content.name = buf.second;
        list.push_back(content);
    }
    return r;
}

int
chfs_client::read(inum ino, size_t size, off_t off, std::string &data)
{
    int r = OK;

    /*
     * your code goes here.
     * note: read using ec->get().
     */
    std::string read_str;
    ec->get(ino, read_str);
    uint32_t read_str_size = read_str.size();
    if (read_str_size < off)
    {
        data = "";
    }
    else if (read_str_size < off + size)
    {
        data = read_str.substr(off);
    }
    else
    {
        data = read_str.substr(off, size);
    }

    return r;
}

int
chfs_client::write(inum ino, size_t size, off_t off, const char *data,
        size_t &bytes_written)
{
    int r = OK;

    /*
     * your code goes here.
     * note: write using ec->put().
     * when off > length of original file, fill the holes with '\0'.
     */
    std::string read_str;
    ec->get(ino, read_str);
    setattr(ino, 0);
    if (off + size > read_str.size())
    {
        read_str.resize(off + size);
    }
    for (uint32_t i = off; i < off + size; ++i)
    {
        read_str[i] = data[i - off];
    }
    ec->put(ino, read_str);
    bytes_written = size;
    return r;
}

int chfs_client::unlink(inum parent, const char *name)
{
    int r = OK;

    /*
     * your code goes here.
     * note: you should remove the file using ec->remove,
     * and update the parent directory content.
     */
    std::list<dirent> files;
    readdir(parent, files);
    for (auto file : files)
    {
        if (file.name == std::string(name))
        {
            ec->remove(file.inum);
            ec->remove_from_dir(parent, file.inum);
            break;
        }
    }

    return r;
}

