typedef short dev_t;

struct driver {
    struct device* (*create)(struct bus_device* parent, void* desc);
    int (*remove)(struct device* device);
    const char* name;
    struct device* instance_list;
    int instance_count;
};

struct dev_ops {
    int (*read)(struct device* dev, void* buff, int count);
    int (*write)(struct device* dev, void* buff, int count);
    int (*ioctl)(struct device* dev, int cmd, void* arg);
    int (*seek)(struct device* dev, int offset, int whence);
};

struct device {
    struct dev_ops* ops;
    struct device* next; //next device instance
    struct bus_device* parent;
    dev_t devno;
};

struct bus_ops {
    //used by drivers to send or recieve over the bus
    int (*sendmsg)(struct bus_device* bus, int addr, void* buff, int count);
    int (*recievemsg)(struct bus_device* bus, int addr, void* buff, int count);
};

struct bus_device {
    struct device base;
    struct bus_ops* bus_ops;
};



