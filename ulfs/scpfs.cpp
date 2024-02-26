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
        if (lstat(local_path.c_str(), stbuf) == -1) {
            std::cerr << "Error getting local file attributes: " << local_path << std::endl;
            return -errno;
        }
    } else {
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

static int scpfs_setxattr(const char* path, const char* name, const char* value, size_t size, int flags) {

    string localPath = LOCAL_DESTINATION + string(path);
    if (file_exists_locally(localPath.c_str())) {
        if (lsetxattr(localPath.c_str(), name, value, size, flags) == -1) {
            cerr << "Error setting local extended attribute: " << localPath << endl;
            return -errno;
        }
    } else {
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
    // std::cerr << "OPENING FILE: " << REMOTE_DESTINATION + string(path) << std::endl;
    log_msg("OPENING FILE: %s\n", (REMOTE_DESTINATION + string(path)));
    sftp_file remoteFile = sftp_open(sftpSession, (REMOTE_DESTINATION + string(path)).c_str(), O_RDONLY, 0);
    if (!remoteFile) {
        cerr << "Error opening file: " << REMOTE_DESTINATION + string(path) << endl;
        return -errno;
    }

    string localPath = LOCAL_DESTINATION + string(path);
    int localFile = open(localPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (localFile == -1) {
        // Failed to create local temporary file
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
            close(localFile);
            sftp_close(remoteFile);
            sftp_free(sftpSession);
            return -errno;
        }
    }

    // Store the local file descriptor in the file info structure
    fi->fh = localFile;

    // Close the remote file
    sftp_close(remoteFile);

    return 0;
}

static int scpfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    int fd;
    int res;
    (void)fi;
    string localPath = LOCAL_DESTINATION + string(path);
    fd = open(localPath.c_str(), O_RDONLY);
    if (fd == -1) {
        log_msg("Error opening file in reading: %s\n", localPath);
        std::cerr << "Error opening file: " << localPath << std::endl;
        return -errno;
    }

    res = pread(fd, buf, size, offset);
    if (res == -1) {
        log_msg("Error reading file: %s\n", localPath);
        std::cerr << "Error reading file for read: " << localPath << std::endl;
        res = -errno;
    }

    close(fd);
    return res;
}

static int scpfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    int fd;
    int res;
    (void)fi;
    string localPath = LOCAL_DESTINATION + string(path);
    fd = open(path, O_WRONLY);
    if (fd == -1) {
        log_msg("Error opening file in writing: %s\n", localPath);
        std::cerr << "Error opening file: " << localPath << std::endl;
        return -errno;
    }

    res = pwrite(fd, buf, size, offset);
    if (res == -1) {
        log_msg("Error writing file: %s\n", localPath);
        std::cerr << "Error writing file: " << localPath << std::endl;
        res = -errno;
    }

    close(fd);
    return res;
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

    int local_file = fi->fh;
    close(local_file);

    if (remove(localpath.c_str()) != 0) {
        cerr << "Error deleting local file: " << localpath << endl;
        return -errno;
    }

    return 0;
}

static struct fuse_operations scpfs_oper = {
    .getattr = scpfs_getattr, .open = scpfs_open, .read = scpfs_read, .write = scpfs_write, .release = scpfs_release, .readdir = scpfs_readdir, .create = scpfs_create};



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