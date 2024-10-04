#include <linux/videodev2.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <jpeglib.h>
#include <unistd.h>

#include "yuyv_to_rgb.h"

#define CAMERA_WIDTH  640
#define CAMERA_HEIGHT 480
#define MMAP_COUNT    2

static void get_next_filename(char *filename, size_t size, const char *folder) {
    unsigned num = 0;
    while (1) {
        snprintf(filename, size, "%s/camera%03d.jpg", folder, num);
        if (access(filename, F_OK) != 0) {
            return;
        }
        num++;
    }
}

static void jpeg_writefile(uint8_t *prgb, int width, int height, const char *folder)
{
    printf("jpeg_writefile start !\n");
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

	char filename[256];
	get_next_filename(filename, sizeof(filename), folder);

    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("fopen");
        return;
    }else{
        printf("jpeg_writefile : fopen success, filename : %s !\n",filename);
    }

    printf("jpeg_writefile : jpeg_stdio_dest(&cinfo, fp) start !\n");
    jpeg_stdio_dest(&cinfo, fp);
    printf("jpeg_writefile : jpeg_stdio_dest(&cinfo, fp) end !\n");

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3; // RGB
    cinfo.in_color_space = JCS_RGB;

    printf("jpeg_writefile : jpeg_set_defaults(&cinfo) start !\n");
    jpeg_set_defaults(&cinfo);
    printf("jpeg_writefile : jpeg_stdio_dest(&cinfo, fp) end !\n");
    printf("jpeg_writefile : jpeg_stdio_dest(&cinfo, fp) start !\n");
    jpeg_start_compress(&cinfo, TRUE);
    printf("jpeg_writefile : jpeg_start_compress(&cinfo, TRUE) end !\n");

    printf("jpeg_writefile : for (int y = 0; y < height; y++) start !\n");
    for (int y = 0; y < height; y++) {
        JSAMPROW row_pointer[1]; // Pointer to a single row
        row_pointer[0] = &prgb[y * width * 3]; // RGB data
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }
    printf("jpeg_writefile : for (int y = 0; y < height; y++) end !\n");

    jpeg_finish_compress(&cinfo);
    fclose(fp);

    jpeg_destroy_compress(&cinfo);
    printf("jpeg_writefile end !\n");
}


static int xioctl(int fd, int request, void *arg)
{
    printf("xioctl start !\n");
    for (; ; ) {
        int ret = ioctl(fd, request, arg);
        if (ret < 0) {
            if (errno == EINTR)
                continue;
            printf("xioctl : errno : %d !\n",errno);
            return -errno;
        }
        break;
    }

    return 0;
    printf("xioctl start !\n");
}

int main(int argc, char *argv[]) {
    const char *camera_dev, *save_folder;
    int fd, width, height, length, ret;
    struct v4l2_format fmt;
    struct v4l2_requestbuffers req;
    struct v4l2_buffer buf;
    enum v4l2_buf_type type;
    void *mmap_p[MMAP_COUNT];
    __u32 mmap_l[MMAP_COUNT];
    uint8_t *yuyvbuf, *rgbbuf;
    struct rusage usage;
    double t;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <camera_device> <save_folder>\n", argv[0]);
        return -1;
    }

    camera_dev = argv[1];
    save_folder = argv[2];

    fd = open(camera_dev, O_RDWR, 0);
    if (fd < 0) {
        perror("open");
        return -1;
    }
    printf("camera %s opened !\n", camera_dev);

    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = CAMERA_WIDTH;
    fmt.fmt.pix.height = CAMERA_HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    ret = xioctl(fd, VIDIOC_S_FMT, &fmt);
    if (ret < 0 || fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_YUYV || fmt.fmt.pix.width <= 0 || fmt.fmt.pix.height <= 0) {
        perror("ioctl(VIDIOC_S_FMT)");
        return -1;
    }
    width = fmt.fmt.pix.width;
    height = fmt.fmt.pix.height;
    length = width * height;

    yuyvbuf = malloc(2 * length);
    if (!yuyvbuf) {
        perror("malloc");
        return -1;
    }

    memset(&req, 0, sizeof(req));
    req.count = MMAP_COUNT;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    ret = xioctl(fd, VIDIOC_REQBUFS, &req);
    if (ret < 0) {
        perror("ioctl(VIDIOC_REQBUFS)");
        return -1;
    }

    for (unsigned i = 0; i < req.count; i++) {
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        ret = xioctl(fd, VIDIOC_QUERYBUF, &buf);
        if (ret < 0) {
            perror("ioctl(VIDIOC_QUERYBUF)");
            return -1;
        }

        mmap_p[i] = mmap(NULL, buf.length, PROT_READ, MAP_SHARED, fd, buf.m.offset);
        if (mmap_p[i] == MAP_FAILED) {
            perror("mmap");
            return -1;
        }
        mmap_l[i] = buf.length;
    }

    for (unsigned i = 0; i < req.count; i++) {
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        ret = xioctl(fd, VIDIOC_QBUF, &buf);
        if (ret < 0) {
            perror("ioctl(VIDIOC_QBUF)");
            return -1;
        }
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = xioctl(fd, VIDIOC_STREAMON, &type);
    if (ret < 0) {
        perror("ioctl(VIDIOC_STREAMON)");
        return -1;
    }

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    while (1) {
        ret = select(fd + 1, &fds, NULL, NULL, NULL);
        if (ret < 0 && errno != EINTR) {
            perror("select");
            return -1;
        }
        if (FD_ISSET(fd, &fds)) {
            memset(&buf, 0, sizeof(buf));
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            ret = xioctl(fd, VIDIOC_DQBUF, &buf);
            if (ret < 0 || buf.bytesused < (__u32)(2 * length)) {
                perror("ioctl(VIDIOC_DQBUF)");
                return -1;
            }
            memcpy(yuyvbuf, mmap_p[buf.index], 2 * length);
            ret = xioctl(fd, VIDIOC_QBUF, &buf);
            if (ret < 0) {
                perror("ioctl(VIDIOC_QBUF)");
                return -1;
            }
            break;
        }
    }

    xioctl(fd, VIDIOC_STREAMOFF, &type);
    for (unsigned i = 0; i < req.count; i++)
        munmap(mmap_p[i], mmap_l[i]);
    close(fd);

    rgbbuf = malloc(3 * length);
    if (!rgbbuf) {
        perror("malloc");
        return -1;
    }

    getrusage(RUSAGE_SELF, &usage);
    t = ((double)usage.ru_utime.tv_sec * 1e+3 +
         (double)usage.ru_utime.tv_usec * 1e-3);

    yuyv_to_rgb(yuyvbuf, rgbbuf, length);

    getrusage(RUSAGE_SELF, &usage);
    t = ((double)usage.ru_utime.tv_sec * 1e+3 +
         (double)usage.ru_utime.tv_usec * 1e-3) - t;
    printf("convert time: %3.3f msec\n", t);

    jpeg_writefile(rgbbuf, width, height, save_folder);

    free(yuyvbuf);
    free(rgbbuf);
    return 0;
}
