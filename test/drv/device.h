
struct driver {
    struct device* (*create)(void* desc);
    const char* name;
};

struct dev_ops {
    int (*read)(struct device* dev, void* buff, int count);
    int (*write)(struct device* dev, void* buff, int count);
    int (*ioctl)(struct device* dev, int cmd, void* arg);
    int (*seek)(struct device* dev, int offset, int whence);
};

struct device {
    struct dev_ops* ops;
    struct device* next;
    struct bus_device* parent;
};

struct bus_ops {
    int (*notify)(struct bus_device* bus, int event, void* arg); //can also be an ioctl
    int (*sendmsg)(struct bus_device* bus, int addr, void* buff, int count);
    int (*recievemsg)(struct bus_device* bus, int addr, void* buff, int count);
};

struct bus_device {
    struct device base;
    struct bus_ops* bus_ops;
    struct device* dev_list;
};

