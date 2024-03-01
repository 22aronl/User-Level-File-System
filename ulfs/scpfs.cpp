#define FUSE_USE_VERSION 30

#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <fuse.h>
#include <iostream>
#include <libssh/libssh.h>
#include <sys/xattr.h>
#include <libssh/sftp.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>
#include <dirent.h>


using namespace std;

// Configuration
#define REMOTE_HOST "alpha-bits.cs.utexas.edu"
#define REMOTE_PORT 22
#define REMOTE_USER "aaronlo"
#define REMOTE_DESTINATION "aos/lab2/"
#define LOCAL_DESTINATION "/tmp/aos/"

ssh_session sshSession;
sftp_session sftpSession;

bool file_exists_locally(const char *path) { return access(path, F_OK) == 0; }

struct bb_state {
    FILE *logfile;
    // char *rootdir;
};

static int scpfs_getattr(const char *path, struct stat *stbuf) {
    string local_path = LOCAL_DESTINATION + string(path);

    if (file_exists_locally(local_path.c_str())) {
        std::cerr << "Local file exists: " << local_path << std::endl;
        if (lstat(local_path.c_str(), stbuf) == -1) {
            std::cerr << "Error getting local file attributes: " << local_path << std::endl;
            return -errno;
        }
    } else {
        std::cerr << "Remote file exists: " << REMOTE_DESTINATION + string(path) << std::endl;
        sftp_attributes attributes = sftp_stat(sftpSession, (REMOTE_DESTINATION + string(path)).c_str());
        if (attributes == NULL) {
            std::cerr << "Error getting remote file attributes: " << REMOTE_DESTINATION + string(path) << std::endl;
            return -errno;
        }

        stbuf->st_mode = attributes->permissions;
        stbuf->st_nlink = 1;
        stbuf->st_uid = attributes->uid;
        stbuf->st_gid = attributes->gid;
        stbuf->st_size = attributes->size;
        stbuf->st_atime = attributes->atime;
        stbuf->st_mtime = attributes->mtime;
        stbuf->st_ctime = attributes->createtime;
        stbuf->st_blksize = 512;
        stbuf->st_blocks = (attributes->size + 511) / 512;

        sftp_attributes_free(attributes);
    }

    return 0;
}

int scpfs_readlink(const char *path, char *buf, size_t size) {
    string localPath = LOCAL_DESTINATION + string(path);
    return readlink(localPath.c_str(), buf, size);
}

int scpfs_mknod(const char *path, mode_t mode, dev_t dev) {
    string localPath = LOCAL_DESTINATION + string(path);
    if (mknod(localPath.c_str(), mode, dev) == -1) {
        cerr << "Error creating local file: " << localPath << endl;
        return -errno;
    }
    return 0;
}

int scpfs_mkdir(const char *path, mode_t mode) {
    string localPath = LOCAL_DESTINATION + string(path);
    if (mkdir(localPath.c_str(), mode) == -1) {
        cerr << "Error creating local directory: " << localPath << endl;
        return -errno;
    }
    return 0;
}

int scpfs_rmdir(const char *path) {
    string localPath = LOCAL_DESTINATION + string(path);
    if (rmdir(localPath.c_str()) == -1) {
        cerr << "Error deleting local directory: " << localPath << endl;
        return -errno;
    }
    return 0;
}

int scpfs_symlink(const char *from, const char *to) {
    string localFrom = LOCAL_DESTINATION + string(from);
    string localTo = LOCAL_DESTINATION + string(to);
    if (symlink(localFrom.c_str(), localTo.c_str()) == -1) {
        cerr << "Error creating local symlink: " << localFrom << " -> " << localTo << endl;
        return -errno;
    }
    return 0;
}

int scpfs_rename(const char *from, const char *to) {
    string localFrom = LOCAL_DESTINATION + string(from);
    string localTo = LOCAL_DESTINATION + string(to);
    if (rename(localFrom.c_str(), localTo.c_str()) == -1) {
        cerr << "Error renaming local file: " << localFrom << " -> " << localTo << endl;
        return -errno;
    }
    return 0;
}

int scpfs_link(const char *from, const char *to) {
    string localFrom = LOCAL_DESTINATION + string(from);
    string localTo = LOCAL_DESTINATION + string(to);
    if (link(localFrom.c_str(), localTo.c_str()) == -1) {
        cerr << "Error creating local hard link: " << localFrom << " -> " << localTo << endl;
        return -errno;
    }
    return 0;
}

int scpfs_chmod(const char *path, mode_t mode) {
    string localPath = LOCAL_DESTINATION + string(path);
    if (chmod(localPath.c_str(), mode) == -1) {
        cerr << "Error changing local file mode: " << localPath << endl;
        return -errno;
    }
    return 0;
}

int scpfs_chown(const char *path, uid_t uid, gid_t gid) {
    string localPath = LOCAL_DESTINATION + string(path);
    if (chown(localPath.c_str(), uid, gid) == -1) {
        cerr << "Error changing local file owner: " << localPath << endl;
        return -errno;
    }
    return 0;
}

int scpfs_truncate(const char *path, off_t size) {
    string localPath = LOCAL_DESTINATION + string(path);
    if (truncate(localPath.c_str(), size) == -1) {
        cerr << "Error truncating local file: " << localPath << endl;
        return -errno;
    }
    return 0;
}

static int scpfs_utime(const char *path, struct utimbuf *ubuf) {
    string localPath = LOCAL_DESTINATION + string(path);
    if (file_exists_locally(localPath.c_str())) {
        if (utime(localPath.c_str(), ubuf) == -1) {
            cerr << "Error setting local file time: " << localPath << endl;
            return -errno;
        }
    } else {
        std::cerr << "Remote time not supported" << std::endl;
        return -ENOTSUP;
    }

    return 0;
}


static int scpfs_unlink(const char *path) {
    string localPath = LOCAL_DESTINATION + string(path);
    if (file_exists_locally(localPath.c_str())) {
        if (unlink(localPath.c_str()) == -1) {
            cerr << "Error deleting local file: " << localPath << endl;
            return -errno;
        }
    } else {
        if (sftp_unlink(sftpSession, (REMOTE_DESTINATION + string(path)).c_str()) != SSH_OK) {
            cerr << "Error deleting remote file: " << REMOTE_DESTINATION + string(path) << endl;
            return -errno;
        }
    }

    return 0;
}

static int scpfs_setxattr(const char* path, const char* name, const char* value, size_t size, int flags) {

    string localPath = LOCAL_DESTINATION + string(path);
    if (file_exists_locally(localPath.c_str())) {
        if (lsetxattr(localPath.c_str(), name, value, size, flags) == -1) {
            cerr << "Error setting local extended attribute: " << localPath << endl;
            return -errno;
        }
    } else {
        std::cerr << "Remote extended attributes not supported" << std::endl;
        return -ENOTSUP;
        // sftp_attributes attr = (sftp_attributes_struct*)malloc(sizeof(struct sftp_attributes_struct));
        // attr->name = name;
        // attr->value = value;
        // attr->size = size;
        // attr->flags = flags;

        // if (sftp_setstat(sftpSession, (REMOTE_DESTINATION + string(path)).c_str(), attr) != SSH_OK) {
        //     sftp_attributes_free(attr);
        //     cerr << "Error setting remote extended attribute: " << REMOTE_DESTINATION + string(path) << endl;
        //     return -errno;
        // }

        // sftp_attributes_free(attr);
    }

    return 0;
}

static int scpfs_getxattr(const char *path, const char *name, char *value, size_t size) {
    string localPath = LOCAL_DESTINATION + string(path);
    return lgetxattr(localPath.c_str(), name, value, size);
}

static int scpfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    // Construct local file path
    std::string localPath = LOCAL_DESTINATION + string(path);

    // Create the local file
    int fd = open(localPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, mode);
    cerr << "CREATING FILE: " << localPath << std::endl;
    if (fd == -1) {
        // Handle error
        cerr << "Error creating local file: " << localPath << endl;
        return -errno;
    }

    // Store the file descriptor in fi->fh
    fi->fh = fd;

    return 0;
}

void log_msg(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);

    vfprintf(((bb_state *) fuse_get_context()->private_data)->logfile, format, ap);
}

static int scpfs_open(const char *path, struct fuse_file_info *fi) {
    std::cerr << "OPENING FILE: " << REMOTE_DESTINATION + string(path) << std::endl;
    // log_msg("OPENING FILE: %s\n", (REMOTE_DESTINATION + string(path)));
    sftp_file remoteFile = sftp_open(sftpSession, (REMOTE_DESTINATION + string(path)).c_str(), O_RDONLY, 0);
    if (!remoteFile) {
        cerr << "Error opening file: " << REMOTE_DESTINATION + string(path) << endl;
        return -errno;
    }

    string localPath = LOCAL_DESTINATION + string(path);
    int localFile = open(localPath.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
    std::cerr << "CREATING FILE: " << localFile << std::endl;
    if (localFile == -1) {
        // Failed to create local temporary file
        cerr << "Error creating local file: " << localPath << endl;
        sftp_close(remoteFile);
        sftp_free(sftpSession);
        return -errno;
    }

    char buffer[8192];
    ssize_t bytesRead;
    while ((bytesRead = sftp_read(remoteFile, buffer, sizeof(buffer))) > 0) {
        ssize_t bytesWritten = write(localFile, buffer, bytesRead);
        if (bytesWritten != bytesRead) {
            // Failed to write data to local temporary file
            
            std::cerr << "ERROR: CLOSING FILE FILE: " << localFile << std::endl;
            close(localFile);
            sftp_close(remoteFile);
            sftp_free(sftpSession);
            return -errno;
        }
    }

    // Store the local file descriptor in the file info structure
    fi->fh = localFile;
    std::cerr << "\tOPENED FILE: " << localFile << std::endl;

    // Close the remote file
    sftp_close(remoteFile);

    return 0;
}

static int scpfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    // int fd;
    // int res;
    // (void)fi;
    // string localPath = LOCAL_DESTINATION + string(path);
    // fd = open(localPath.c_str(), O_RDONLY);
    // if (fd == -1) {
    //     log_msg("Error opening file in reading: %s\n", localPath);
    //     std::cerr << "Error opening file: " << localPath << std::endl;
    //     return -errno;
    // }
    std::cerr << "\tREADFILE FILE: " << fi->fh << std::endl;
    int res = pread(fi->fh, buf, size, offset);
    if (res == -1) {
        // log_msg("Error reading file: %s\n", "hi");
        std::cerr << "Error reading file for read: " << res << std::endl;
        res = -errno;
    }

    // close(fd);
    return res;
}

static int scpfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    // int fd;
    // int res;
    // (void)fi;
    std::cerr << "WRITING FILE: " << LOCAL_DESTINATION + string(path) << std::endl;
    // string localPath = LOCAL_DESTINATION + string(path);
    // fd = open(path, O_WRONLY);
    // if (fd == -1) {
    //     log_msg("Error opening file in writing: %s\n", localPath);
    //     std::cerr << "Error opening file: " << localPath << std::endl;
    //     return -errno;
    // }

    int res = pwrite(fi->fh, buf, size, offset);
    // if (res == -1) {
    //     log_msg("Error writing file: %s\n", localPath);
    //     std::cerr << "Error writing file: " << localPath << std::endl;
    //     res = -errno;
    // }

    // close(fd);
    return res;
}

int scpfs_statfs(const char *path, struct statvfs *stbuf) {
    string localPath = LOCAL_DESTINATION + string(path);
    return statvfs(localPath.c_str(), stbuf);
}


static int scpfs_release(const char *path, struct fuse_file_info *fi) {
    (void)path;
    (void)fi;

    // Close the local temporary file

    string localpath = LOCAL_DESTINATION + string(path);

    // Upload the file to remote
    sftp_file remoteFile = sftp_open(sftpSession, (REMOTE_DESTINATION + string(path)).c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
    if (remoteFile) {
        cerr << "WRITING OUT FILE: " << REMOTE_DESTINATION + string(path) << std::endl;
        ifstream localFile(localpath);
        char buffer[1024];
        int nbytes;
        while ((nbytes = localFile.readsome(buffer, sizeof(buffer))) > 0) {
            sftp_write(remoteFile, buffer, nbytes);
        }
        sftp_close(remoteFile);
        localFile.close();
    } else {
        cerr << "Error opening remote file: " << REMOTE_DESTINATION + string(path) << endl;
        return -errno;
    }
    std::cerr << "CLOSING FILE: " << fi->fh << std::endl;
    int local_file = fi->fh;
    close(local_file);

    std::cerr << "DELETING FILE: " << localpath << std::endl;

    if (remove(localpath.c_str()) != 0) {
        cerr << "Error deleting local file: " << localpath << endl;
        return -errno;
    }

    return 0;
}

int scpfs_fsync(const char *path, int isdatasync, struct fuse_file_info *fi) {
    if(isdatasync)
        return fdatasync(fi->fh);
    else
        return fsync(fi->fh);
}

int scpfs_listxattr(const char *path, char *list, size_t size) {
    string localPath = LOCAL_DESTINATION + string(path);
    return listxattr(localPath.c_str(), list, size);
}

int scpfs_opendir(const char *path, struct fuse_file_info *fi) {
    string localPath = LOCAL_DESTINATION + string(path);
    
    sftp_dir dir = sftp_opendir(sftpSession, (REMOTE_DESTINATION + string(path)).c_str());
    if (dir == NULL) {
        cerr << "Error opening remote directory: " << REMOTE_DESTINATION + string(path) << endl;
        return -errno;
    }


    if(mkdir(localPath.c_str(), 0755) == -1) {
        cerr << "Error creating local directory: " << localPath << endl;
        return -errno;
    }

    fi->fh = (intptr_t)dir;

    return 0;
}

static int scpfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    (void)offset;
    (void)fi;

    sftp_dir dir;
    sftp_attributes attributes;

    string remoteDir = REMOTE_DESTINATION + string(path);

    dir = sftp_opendir(sftpSession, remoteDir.c_str());
    if (dir == NULL) {
        cerr << "Error opening directory: " << remoteDir << endl;
        return -errno;
    }

    while ((attributes = sftp_readdir(sftpSession, dir)) != NULL) {
        filler(buf, attributes->name, NULL, 0);
        cout << "File: " << attributes->name << endl;
        sftp_attributes_free(attributes);
    }

    sftp_closedir(dir);

    return 0;
}

int scpfs_releasedir(const char *path, struct fuse_file_info *fi) {
    sftp_dir dir = (sftp_dir)(intptr_t)fi->fh;
    sftp_closedir(dir);

    string localPath = LOCAL_DESTINATION + string(path);
    if (rmdir(localPath.c_str()) == -1) {
        cerr << "Error deleting local directory: " << localPath << endl;
        return -errno;
    }
    return 0;
}

int scpfs_access(const char *path, int mask) {
    string localPath = LOCAL_DESTINATION + string(path);
    return access(localPath.c_str(), mask);
}

int scpfs_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi) {
    return ftruncate(fi->fh, offset);
}

int scpfs_fgetattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    return fstat(fi->fh, stbuf);
}

static struct fuse_operations scpfs_oper = {
    .getattr = scpfs_getattr, 
    .readlink = scpfs_readlink,
    .mknod = scpfs_mknod,
    .mkdir = scpfs_mkdir,
    .unlink = scpfs_unlink, 
    .rmdir = scpfs_rmdir,
    .symlink = scpfs_symlink,
    .rename = scpfs_rename,
    .link = scpfs_link,
    .chmod = scpfs_chmod,
    .chown = scpfs_chown,
    .truncate = scpfs_truncate,
    .utime = scpfs_utime,
    .open = scpfs_open, 
    .read = scpfs_read, 
    .write = scpfs_write,
    .statfs = scpfs_statfs,
    .release = scpfs_release, 
    .setxattr = scpfs_setxattr,
    // .getxattr = scpfs_getxattr,
    .listxattr = scpfs_listxattr,
    // .opendir = scpfs_opendir,
    .readdir = scpfs_readdir,
    // .releasedir = scpfs_releasedir,
    .access = scpfs_access,
    .create = scpfs_create,
    .ftruncate = scpfs_ftruncate,
    .fgetattr = scpfs_fgetattr
    };



int main(int argc, char *argv[]) {
    sshSession = ssh_new();
    if (sshSession == NULL) {
        cerr << "Error creating SSH session" << endl;
        return -1;
    }

    ssh_options_set(sshSession, SSH_OPTIONS_HOST, REMOTE_HOST);
    ssh_options_set(sshSession, SSH_OPTIONS_USER, REMOTE_USER);

    // Connect to the SSH server
    int rc = ssh_connect(sshSession);
    if (rc != SSH_OK) {
        std::cerr << "Error connecting to server: " << ssh_get_error(sshSession) << std::endl;
        ssh_free(sshSession);
        return 1;
    }

    // Authenticate using public key
    rc = ssh_userauth_publickey_auto(sshSession, NULL, NULL);
    if (rc != SSH_AUTH_SUCCESS) {
        std::cerr << "Authentication failed: " << ssh_get_error(sshSession) << std::endl;
        ssh_disconnect(sshSession);
        ssh_free(sshSession);
        return 1;
    }

    sftpSession = sftp_new(sshSession);
    if (sftpSession == NULL) {
        std::cerr << "Error getting sftp session" << std::endl;
        ssh_disconnect(sshSession);
        ssh_free(sshSession);
        return 1;
    }

    rc = sftp_init(sftpSession);
    if (rc != SSH_OK) {
        std::cerr << "Error initializing SFTP session: " << ssh_get_error(sshSession) << std::endl;
        sftp_free(sftpSession);
        ssh_disconnect(sshSession);
        ssh_free(sshSession);
        return 1;
    }

    bb_state *bb_data;
    bb_data = (bb_state*) malloc(sizeof(struct bb_state));
    if (bb_data == NULL) {
        perror("main calloc");
        abort();
    }
    FILE* file = fopen("scpfs.log", "w");
    if (file == NULL) {
        perror("main fopen");
        abort();
    }
    setvbuf(file, NULL, _IOLBF, 0);
    
    bb_data->logfile = file;

    return fuse_main(argc, argv, &scpfs_oper, bb_data);
} // g++ -std=c++11 -D_FILE_OFFSET_BITS=64 -o scpfs scpfs.cpp -lfuse -lssh -lpthread -lstdc++