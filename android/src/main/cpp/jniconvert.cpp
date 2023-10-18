#include <jni.h>
#include <fstream>
#include <string>
#include <sstream>
#include <string>

#include "thneedmodel.h"

#include <cassert>
#include <cstring>
#include <map>

#include "clutil.h"
#include "timing.h"

#include "json11.hpp"
#include "util.h"

map<pair<cl_kernel, int>, string> g_args;
map<pair<cl_kernel, int>, int> g_args_size;
map<cl_program, string> g_program_source;

#include <dlfcn.h>
#include <sys/mman.h>

#include <cassert>
#include <cerrno>
#include <cstring>
#include <map>
#include <string>

#define QCOM2

Thneed *g_thneed = NULL;
int g_fd = -1;

#include <dlfcn.h>
#include "CL/cl.h"

// Define function pointer types
typedef cl_program (*clCreateProgramWithSource_t)(cl_context, cl_uint, const char **, const size_t *, cl_int *);
typedef cl_int (*clBuildProgram_t)(cl_program, cl_uint, const cl_device_id *, const char *, void (*pfn_notify)(cl_program, void *), void *);
typedef cl_program (*clCreateProgramWithBinary_t)(cl_context, cl_uint, const cl_device_id *, const size_t *, const unsigned char **, cl_int *, cl_int *);
typedef cl_int (*clGetPlatformIDs_t)(cl_uint, cl_platform_id *, cl_uint *);
typedef cl_int (*clGetDeviceIDs_t)(cl_platform_id, cl_device_type, cl_uint, cl_device_id *, cl_uint *);
typedef cl_mem (*clCreateBuffer_t)(cl_context, cl_mem_flags, size_t, void *, cl_int *);
typedef cl_mem (*clCreateImage_t)(cl_context, cl_mem_flags, const cl_image_format *, const cl_image_desc *, void *, cl_int *);
typedef void * (*clEnqueueMapBuffer_t)(cl_command_queue, cl_mem, cl_bool, cl_map_flags, size_t, size_t, cl_uint, const cl_event *, cl_event *, cl_int *);
typedef cl_int (*clFinish_t)(cl_command_queue);
typedef cl_int (*clGetMemObjectInfo_t)(cl_mem, cl_mem_info, size_t, void *, size_t *);
typedef cl_int (*clGetImageInfo_t)(cl_mem, cl_image_info, size_t, void *, size_t *);
typedef cl_context (*clCreateContext_t)(const cl_context_properties *, cl_uint,const  cl_device_id *, void (CL_CALLBACK *)(const char *, const void *, size_t, void *), void *, 	cl_int *);
typedef	cl_command_queue (*clCreateCommandQueueWithProperties_t)(	cl_context , 	cl_device_id , 	const 	cl_queue_properties * , 	cl_int * );
typedef	cl_int (*clEnqueueWriteBuffer_t)(	cl_command_queue , 	cl_mem , 	cl_bool , 	size_t , 	size_t , 	const void * , 	cl_uint , 	const 	cl_event * , 	cl_event * );
typedef	cl_int (*clEnqueueReadBuffer_t)(	cl_command_queue , 	cl_mem , 	cl_bool , 	size_t , 	size_t , 	void * , 	cl_uint , 	const 	cl_event * , 	cl_event * );

// Load the OpenCL library
void* opencl_library = dlopen("libOpenCL.so", RTLD_LAZY | RTLD_LOCAL);

// Get function pointers
auto p_clCreateProgramWithSource = reinterpret_cast<clCreateProgramWithSource_t>(dlsym(opencl_library,"clCreateProgramWithSource"));
auto p_clBuildProgram = reinterpret_cast<clBuildProgram_t>(dlsym(opencl_library,"clBuildProgram"));
auto p_clCreateProgramWithBinary = reinterpret_cast<clCreateProgramWithBinary_t>(dlsym(opencl_library,"clCreateProgramWithBinary"));
auto p_clGetPlatformIDs = reinterpret_cast<clGetPlatformIDs_t>(dlsym(opencl_library,"clGetPlatformIDs"));
auto p_clGetDeviceIDs = reinterpret_cast<clGetDeviceIDs_t>(dlsym(opencl_library,"clGetDeviceIDs"));
auto p_clCreateBuffer = reinterpret_cast<clCreateBuffer_t>(dlsym(opencl_library,"clCreateBuffer"));
auto p_clCreateImage = reinterpret_cast<clCreateImage_t>(dlsym(opencl_library,"clCreateImage"));
auto p_clEnqueueMapBuffer = reinterpret_cast<clEnqueueMapBuffer_t>(dlsym(opencl_library,"clEnqueueMapBuffer"));
auto p_clFinish = reinterpret_cast<clFinish_t>(dlsym(opencl_library,"clFinish"));
auto p_clGetMemObjectInfo = reinterpret_cast<clGetMemObjectInfo_t>(dlsym(opencl_library,"clGetMemObjectInfo"));
auto p_clGetImageInfo = reinterpret_cast<clGetImageInfo_t>(dlsym(opencl_library,"clGetImageInfo"));
auto p_clCreateContext = reinterpret_cast<clCreateContext_t>(dlsym(opencl_library,"clCreateContext"));
auto p_clCreateCommandQueueWithProperties = reinterpret_cast<clCreateCommandQueueWithProperties_t>(dlsym(opencl_library,"clCreateCommandQueueWithProperties"));
auto p_clEnqueueWriteBuffer = reinterpret_cast<clEnqueueWriteBuffer_t>(dlsym(opencl_library,"clEnqueueWriteBuffer"));
auto p_clEnqueueReadBuffer = reinterpret_cast<clEnqueueReadBuffer_t>(dlsym(opencl_library,"clEnqueueReadBuffer"));

// Define more function pointer types
typedef cl_kernel (*clCreateKernel_t)(cl_program, const char *, cl_int *);
typedef cl_int (*clGetKernelArgInfo_t)(cl_kernel, cl_uint, cl_kernel_arg_info, size_t, void *, size_t *);
typedef cl_int (*clEnqueueNDRangeKernel_t)(cl_command_queue, cl_kernel, cl_uint, const size_t *, const size_t *, const size_t *, cl_uint, const cl_event *, cl_event *);
typedef cl_int (*clGetKernelInfo_t)(cl_kernel, cl_kernel_info, size_t, void *, size_t *);
typedef cl_int (*clSetKernelArg_t)(cl_kernel, cl_uint, size_t, const void *);

// Get more function pointers
auto p_clCreateKernel = reinterpret_cast<clCreateKernel_t>(dlsym(opencl_library,"clCreateKernel"));
auto p_clGetKernelArgInfo = reinterpret_cast<clGetKernelArgInfo_t>(dlsym(opencl_library,"clGetKernelArgInfo"));
auto p_clEnqueueNDRangeKernel = reinterpret_cast<clEnqueueNDRangeKernel_t>(dlsym(opencl_library,"clEnqueueNDRangeKernel"));
auto p_clGetKernelInfo = reinterpret_cast<clGetKernelInfo_t>(dlsym(opencl_library,"clGetKernelInfo"));
auto p_clSetKernelArg = reinterpret_cast<clSetKernelArg_t>(dlsym(opencl_library,"clSetKernelArg"));

// Now you can use these function pointers as if they were the original functions
// For example:
// cl_kernel kernel = (*p_clCreateKernel)(program, "my_kernel", &err);


// Now you can use these function pointers as if they were the original functions
// For example:
// cl_context context = (*p_clCreateContext)(NULL, 1, &device_id, NULL, NULL, &err);

void hexdump(uint8_t *d, int len) {
    assert((len%4) == 0);
    printf("  dumping %p len 0x%x\n", d, len);
    for (int i = 0; i < len/4; i++) {
        if (i != 0 && (i%0x10) == 0) printf("\n");
        printf("%8x ", d[i]);
    }
    printf("\n");
}

extern map<cl_program, string> g_program_source;

template <typename Func, typename Id, typename Name>
std::string get_info(Func get_info_func, Id id, Name param_name) {
    size_t size = 0;
    CL_CHECK(get_info_func(id, param_name, 0, NULL, &size));
    std::string info(size, '\0');
    CL_CHECK(get_info_func(id, param_name, size, info.data(), NULL));
    return info;
}inline std::string get_platform_info(cl_platform_id id, cl_platform_info name) { return get_info(&clGetPlatformInfo, id, name); }

cl_program cl_program_from_source(cl_context ctx, cl_device_id device_id, const std::string& src, const char* args) {
    const char *csrc = src.c_str();
    cl_program prg = CL_CHECK_ERR((*p_clCreateProgramWithSource)(ctx, 1, &csrc, NULL, &err));
    if (int err = (*p_clBuildProgram)(prg, 1, &device_id, args, NULL, NULL); err != 0) {
        assert(0);
    }
    return prg;
}

cl_program cl_program_from_binary(cl_context ctx, cl_device_id device_id, const uint8_t* binary, size_t length, const char* args) {
    cl_program prg = CL_CHECK_ERR((*p_clCreateProgramWithBinary)(ctx, 1, &device_id, &length, &binary, NULL, &err));
    if (int err = (*p_clBuildProgram)(prg, 1, &device_id, args, NULL, NULL); err != 0) {
        assert(0);
    }
    return prg;
}

cl_device_id cl_get_device_id(cl_device_type device_type) {
    cl_uint num_platforms = 0;
    CL_CHECK((*p_clGetPlatformIDs)(0, NULL, &num_platforms));
    std::unique_ptr<cl_platform_id[]> platform_ids = std::make_unique<cl_platform_id[]>(num_platforms);
    CL_CHECK((*p_clGetPlatformIDs)(num_platforms, &platform_ids[0], NULL));

    for (size_t i = 0; i < num_platforms; ++i) {
        // Get first device
        if (cl_device_id device_id = NULL; (*p_clGetDeviceIDs)(platform_ids[i], device_type, 1, &device_id, NULL) == 0 && device_id) {
            return device_id;
        }
    }
    assert(0);
    return nullptr;
}

std::string readFileIntoString(const char *filepath) {
    std::ifstream ifs(filepath);
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    return buffer.str();
}

void Thneed::load(const char *filename) {
    printf("Thneed::load: loading from %s\n", filename);

    string buf = readFileIntoString(filename);
    int jsz = *(int *)buf.data();
    string jsonerr;
    string jj(buf.data() + sizeof(int), jsz);
    json11::Json jdat = json11::Json::parse(jj, jsonerr);

    map<cl_mem, cl_mem> real_mem;
    real_mem[NULL] = NULL;

    int ptr = sizeof(int)+jsz;
    for (auto &obj : jdat["objects"].array_items()) {
        auto mobj = obj.object_items();
        int sz = mobj["size"].int_value();
        cl_mem clbuf = NULL;

        if (mobj["buffer_id"].string_value().size() > 0) {
            // image buffer must already be allocated
            clbuf = real_mem[*(cl_mem*)(mobj["buffer_id"].string_value().data())];
            assert(mobj["needs_load"].bool_value() == false);
        } else {
            if (mobj["needs_load"].bool_value()) {
                clbuf = (*p_clCreateBuffer)(context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_WRITE, sz, &buf[ptr], NULL);
                if (debug >= 1) printf("loading %p %d @ 0x%X\n", clbuf, sz, ptr);
                ptr += sz;
            } else {
                // TODO: is there a faster way to init zeroed out buffers?
                void *host_zeros = calloc(sz, 1);
                clbuf = (*p_clCreateBuffer)(context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_WRITE, sz, host_zeros, NULL);
                free(host_zeros);
            }
        }
        assert(clbuf != NULL);

        if (mobj["arg_type"] == "image2d_t" || mobj["arg_type"] == "image1d_t") {
            cl_image_desc desc = {0};
            desc.image_type = (mobj["arg_type"] == "image2d_t") ? CL_MEM_OBJECT_IMAGE2D : CL_MEM_OBJECT_IMAGE1D_BUFFER;
            desc.image_width = mobj["width"].int_value();
            desc.image_height = mobj["height"].int_value();
            desc.image_row_pitch = mobj["row_pitch"].int_value();
            assert(sz == desc.image_height*desc.image_row_pitch);
#ifdef QCOM2
            desc.buffer = clbuf;
#else
            // TODO: we are creating unused buffers on PC
            clReleaseMemObject(clbuf);
#endif
            cl_image_format format = {0};
            format.image_channel_order = CL_RGBA;
            format.image_channel_data_type = mobj["float32"].bool_value() ? CL_FLOAT : CL_HALF_FLOAT;

            cl_int errcode;

#ifndef QCOM2
            if (mobj["needs_load"].bool_value()) {
                clbuf = clCreateImage(context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_WRITE, &format, &desc, &buf[ptr-sz], &errcode);
            } else {
                clbuf = clCreateImage(context, CL_MEM_READ_WRITE, &format, &desc, NULL, &errcode);
            }
#else
            clbuf = (*p_clCreateImage)(context, CL_MEM_READ_WRITE, &format, &desc, NULL, &errcode);
#endif
            /*if (clbuf == NULL) {
                printf("clError: %s create image %zux%zu rp %zu with buffer %p\n", cl_get_error_string(errcode),
                       desc.image_width, desc.image_height, desc.image_row_pitch, desc.buffer);
            }*/
            assert(clbuf != NULL);
        }

        real_mem[*(cl_mem*)(mobj["id"].string_value().data())] = clbuf;
    }

    map<string, cl_program> g_programs;
    for (const auto &[name, source] : jdat["programs"].object_items()) {
        if (debug >= 1) printf("building %s with size %zu\n", name.c_str(), source.string_value().size());
        g_programs[name] = cl_program_from_source(context, device_id, source.string_value());
    }

    for (auto &obj : jdat["inputs"].array_items()) {
        auto mobj = obj.object_items();
        int sz = mobj["size"].int_value();
        cl_mem aa = real_mem[*(cl_mem*)(mobj["buffer_id"].string_value().data())];
        input_clmem.push_back(aa);
        input_sizes.push_back(sz);
        printf("Thneed::load: adding input %s with size %d\n", mobj["name"].string_value().data(), sz);

        cl_int cl_err;
        void *ret = (*p_clEnqueueMapBuffer)(command_queue, aa, CL_TRUE, CL_MAP_WRITE, 0, sz, 0, NULL, NULL, &cl_err);
        //if (cl_err != CL_SUCCESS) printf("clError: %s map %p %d\n", cl_get_error_string(cl_err), aa, sz);
        assert(cl_err == CL_SUCCESS);
        inputs.push_back(ret);
    }

    for (auto &obj : jdat["outputs"].array_items()) {
        auto mobj = obj.object_items();
        int sz = mobj["size"].int_value();
        printf("Thneed::save: adding output with size %d\n", sz);
        // TODO: support multiple outputs
        output = real_mem[*(cl_mem*)(mobj["buffer_id"].string_value().data())];
        assert(output != NULL);
    }

    for (auto &obj : jdat["binaries"].array_items()) {
        string name = obj["name"].string_value();
        size_t length = obj["length"].int_value();
        if (debug >= 1) printf("binary %s with size %zu\n", name.c_str(), length);
        g_programs[name] = cl_program_from_binary(context, device_id, (const uint8_t*)&buf[ptr], length);
        ptr += length;
    }

    for (auto &obj : jdat["kernels"].array_items()) {
        auto gws = obj["global_work_size"];
        auto lws = obj["local_work_size"];
        auto kk = shared_ptr<CLQueuedKernel>(new CLQueuedKernel(this));

        kk->name = obj["name"].string_value();
        kk->program = g_programs[kk->name];
        kk->work_dim = obj["work_dim"].int_value();
        for (int i = 0; i < kk->work_dim; i++) {
            kk->global_work_size[i] = gws[i].int_value();
            kk->local_work_size[i] = lws[i].int_value();
        }
        kk->num_args = obj["num_args"].int_value();
        for (int i = 0; i < kk->num_args; i++) {
            string arg = obj["args"].array_items()[i].string_value();
            int arg_size = obj["args_size"].array_items()[i].int_value();
            kk->args_size.push_back(arg_size);
            if (arg_size == 8) {
                cl_mem val = *(cl_mem*)(arg.data());
                val = real_mem[val];
                kk->args.push_back(string((char*)&val, sizeof(val)));
            } else {
                kk->args.push_back(arg);
            }
        }
        kq.push_back(kk);
    }

    (*p_clFinish)(command_queue);
}

// *********** ioctl interceptor ***********

extern "C" {

int __ioctl(int filedes, unsigned long request, void *argp) {
    request &= 0xFFFFFFFF;  // needed on QCOM2
    Thneed *thneed = g_thneed;

    // save the fd
    if (request == IOCTL_KGSL_GPUOBJ_ALLOC) g_fd = filedes;

    // note that this runs always, even without a thneed object
    if (request == IOCTL_KGSL_DRAWCTXT_CREATE) {
        struct kgsl_drawctxt_create *create = (struct kgsl_drawctxt_create *)argp;
        create->flags &= ~KGSL_CONTEXT_PRIORITY_MASK;
        create->flags |= 6 << KGSL_CONTEXT_PRIORITY_SHIFT;   // priority from 1-15, 1 is max priority
        printf("IOCTL_KGSL_DRAWCTXT_CREATE: creating context with flags 0x%x\n", create->flags);
    }

    if (thneed != NULL) {
        if (request == IOCTL_KGSL_GPU_COMMAND) {
            struct kgsl_gpu_command *cmd = (struct kgsl_gpu_command *)argp;
            if (thneed->record) {
                thneed->timestamp = cmd->timestamp;
                thneed->context_id = cmd->context_id;
                thneed->cmds.push_back(unique_ptr<CachedCommand>(new CachedCommand(thneed, cmd)));
            }
            if (thneed->debug >= 1) {
                printf("IOCTL_KGSL_GPU_COMMAND(%2zu): flags: 0x%lx    context_id: %u  timestamp: %u  numcmds: %d  numobjs: %d\n",
                       thneed->cmds.size(),
                       cmd->flags,
                       cmd->context_id, cmd->timestamp, cmd->numcmds, cmd->numobjs);
            }
        } else if (request == IOCTL_KGSL_GPUOBJ_SYNC) {
            struct kgsl_gpuobj_sync *cmd = (struct kgsl_gpuobj_sync *)argp;
            struct kgsl_gpuobj_sync_obj *objs = (struct kgsl_gpuobj_sync_obj *)(cmd->objs);

            if (thneed->debug >= 2) {
                printf("IOCTL_KGSL_GPUOBJ_SYNC count:%d ", cmd->count);
                for (int i = 0; i < cmd->count; i++) {
                    printf(" -- offset:0x%lx len:0x%lx id:%d op:%d  ", objs[i].offset, objs[i].length, objs[i].id, objs[i].op);
                }
                printf("\n");
            }

            if (thneed->record) {
                thneed->cmds.push_back(unique_ptr<CachedSync>(new CachedSync(thneed, string((char *)objs, sizeof(struct kgsl_gpuobj_sync_obj)*cmd->count))));
            }
        } else if (request == IOCTL_KGSL_DEVICE_WAITTIMESTAMP_CTXTID) {
            struct kgsl_device_waittimestamp_ctxtid *cmd = (struct kgsl_device_waittimestamp_ctxtid *)argp;
            if (thneed->debug >= 1) {
                printf("IOCTL_KGSL_DEVICE_WAITTIMESTAMP_CTXTID: context_id: %d  timestamp: %d  timeout: %d\n",
                       cmd->context_id, cmd->timestamp, cmd->timeout);
            }
        } else if (request == IOCTL_KGSL_SETPROPERTY) {
            if (thneed->debug >= 1) {
                struct kgsl_device_getproperty *prop = (struct kgsl_device_getproperty *)argp;
                printf("IOCTL_KGSL_SETPROPERTY: 0x%x sizebytes:%zu\n", prop->type, prop->sizebytes);
                if (thneed->debug >= 2) {
                    hexdump((uint8_t *)prop->value, prop->sizebytes);
                    if (prop->type == KGSL_PROP_PWR_CONSTRAINT) {
                        struct kgsl_device_constraint *constraint = (struct kgsl_device_constraint *)prop->value;
                        hexdump((uint8_t *)constraint->data, constraint->size);
                    }
                }
            }
        } else if (request == IOCTL_KGSL_DRAWCTXT_CREATE || request == IOCTL_KGSL_DRAWCTXT_DESTROY) {
            // this happens
        } else if (request == IOCTL_KGSL_GPUOBJ_ALLOC || request == IOCTL_KGSL_GPUOBJ_FREE) {
            // this happens
        } else {
            if (thneed->debug >= 1) {
                printf("other ioctl %lx\n", request);
            }
        }
    }

    int ret = ioctl(filedes, request, argp);
    // NOTE: This error message goes into stdout and messes up pyenv
    // if (ret != 0) printf("ioctl returned %d with errno %d\n", ret, errno);
    return ret;
}

}

// *********** GPUMalloc ***********

GPUMalloc::GPUMalloc(int size, int fd) {
    struct kgsl_gpuobj_alloc alloc;
    memset(&alloc, 0, sizeof(alloc));
    alloc.size = size;
    alloc.flags = 0x10000a00;
    __ioctl(fd, IOCTL_KGSL_GPUOBJ_ALLOC, &alloc);
    void *addr = mmap64(NULL, alloc.mmapsize, 0x3, 0x1, fd, alloc.id*0x1000);
    assert(addr != MAP_FAILED);

    base = (uint64_t)addr;
    remaining = size;
}

GPUMalloc::~GPUMalloc() {
    // TODO: free the GPU malloced area
}

void *GPUMalloc::alloc(int size) {
    void *ret = (void*)base;
    size = (size+0xff) & (~0xFF);
    assert(size <= remaining);
    remaining -= size;
    base += size;
    return ret;
}

// *********** CachedSync, at the ioctl layer ***********

void CachedSync::exec() {
    struct kgsl_gpuobj_sync cmd;

    cmd.objs = (uint64_t)data.data();
    cmd.obj_len = data.length();
    cmd.count = data.length() / sizeof(struct kgsl_gpuobj_sync_obj);

    int ret = __ioctl(thneed->fd, IOCTL_KGSL_GPUOBJ_SYNC, &cmd);
    assert(ret == 0);
}

// *********** CachedCommand, at the ioctl layer ***********

CachedCommand::CachedCommand(Thneed *lthneed, struct kgsl_gpu_command *cmd) {
    thneed = lthneed;
    assert(cmd->numsyncs == 0);

    memcpy(&cache, cmd, sizeof(cache));

    if (cmd->numcmds > 0) {
        cmds = make_unique<struct kgsl_command_object[]>(cmd->numcmds);
        memcpy(cmds.get(), (void *)cmd->cmdlist, sizeof(struct kgsl_command_object)*cmd->numcmds);
        cache.cmdlist = (uint64_t)cmds.get();
        for (int i = 0; i < cmd->numcmds; i++) {
            void *nn = thneed->ram->alloc(cmds[i].size);
            memcpy(nn, (void*)cmds[i].gpuaddr, cmds[i].size);
            cmds[i].gpuaddr = (uint64_t)nn;
        }
    }

    if (cmd->numobjs > 0) {
        objs = make_unique<struct kgsl_command_object[]>(cmd->numobjs);
        memcpy(objs.get(), (void *)cmd->objlist, sizeof(struct kgsl_command_object)*cmd->numobjs);
        cache.objlist = (uint64_t)objs.get();
        for (int i = 0; i < cmd->numobjs; i++) {
            void *nn = thneed->ram->alloc(objs[i].size);
            memset(nn, 0, objs[i].size);
            objs[i].gpuaddr = (uint64_t)nn;
        }
    }

    kq = thneed->ckq;
    thneed->ckq.clear();
}

void CachedCommand::exec() {
    cache.timestamp = ++thneed->timestamp;
    int ret = __ioctl(thneed->fd, IOCTL_KGSL_GPU_COMMAND, &cache);

    if (thneed->debug >= 1) printf("CachedCommand::exec got %d\n", ret);

    if (thneed->debug >= 2) {
        for (auto &it : kq) {
            it->debug_print(false);
        }
    }

    assert(ret == 0);
}

// *********** Thneed ***********

Thneed::Thneed(bool do_clinit, cl_context _context) {
    // TODO: QCOM2 actually requires a different context
    //context = _context;
    if (do_clinit) clinit();
    assert(g_fd != -1);
    fd = g_fd;
    ram = make_unique<GPUMalloc>(0x80000, fd);
    timestamp = -1;
    g_thneed = this;
    char *thneed_debug_env = getenv("THNEED_DEBUG");
    debug = (thneed_debug_env != NULL) ? atoi(thneed_debug_env) : 0;
}

void Thneed::wait() {
    struct kgsl_device_waittimestamp_ctxtid wait;
    wait.context_id = context_id;
    wait.timestamp = timestamp;
    wait.timeout = -1;

    uint64_t tb = nanos_since_boot();
    int wret = __ioctl(fd, IOCTL_KGSL_DEVICE_WAITTIMESTAMP_CTXTID, &wait);
    uint64_t te = nanos_since_boot();

    if (debug >= 1) printf("wait %d after %lu us\n", wret, (te-tb)/1000);
}

void Thneed::execute(float **finputs, float *foutput, bool slow) {
    uint64_t tb, te;
    if (debug >= 1) tb = nanos_since_boot();

    // ****** copy inputs
    copy_inputs(finputs, true);

    // ****** run commands
    int i = 0;
    for (auto &it : cmds) {
        ++i;
        if (debug >= 1) printf("run %2d @ %7lu us: ", i, (nanos_since_boot()-tb)/1000);
        it->exec();
        if ((i == cmds.size()) || slow) wait();
    }

    // ****** copy outputs
    copy_output(foutput);

    if (debug >= 1) {
        te = nanos_since_boot();
        printf("model exec in %lu us\n", (te-tb)/1000);
    }
}

void Thneed::stop() {
    //printf("Thneed::stop: recorded %lu commands\n", cmds.size());
    record = false;
}

void Thneed::clinit() {
    device_id = cl_get_device_id(CL_DEVICE_TYPE_DEFAULT);
    if (context == NULL) context = CL_CHECK_ERR((*p_clCreateContext)(NULL, 1, &device_id, NULL, NULL, &err));
    //cl_command_queue_properties props[3] = {CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0};
    cl_command_queue_properties props[3] = {CL_QUEUE_PROPERTIES, 0, 0};
    command_queue = CL_CHECK_ERR((*p_clCreateCommandQueueWithProperties)(context, device_id, props, &err));
    printf("Thneed::clinit done\n");
}

cl_int Thneed::clexec() {
    if (debug >= 1) printf("Thneed::clexec: running %lu queued kernels\n", kq.size());
    for (auto &k : kq) {
        if (record) ckq.push_back(k);
        cl_int ret = k->exec();
        assert(ret == CL_SUCCESS);
    }
    return (*p_clFinish)(command_queue);
}

void Thneed::copy_inputs(float **finputs, bool internal) {
    for (int idx = 0; idx < inputs.size(); ++idx) {
        if (debug >= 1) printf("copying %lu -- %p -> %p (cl %p)\n", input_sizes[idx], finputs[idx], inputs[idx], input_clmem[idx]);

        if (internal) {
            // if it's internal, using memcpy is fine since the buffer sync is cached in the ioctl layer
            if (finputs[idx] != NULL) memcpy(inputs[idx], finputs[idx], input_sizes[idx]);
        } else {
            if (finputs[idx] != NULL) CL_CHECK((*p_clEnqueueWriteBuffer)(command_queue, input_clmem[idx], CL_TRUE, 0, input_sizes[idx], finputs[idx], 0, NULL, NULL));
        }
    }
}

void Thneed::copy_output(float *foutput) {
    if (output != NULL) {
        size_t sz;
        (*p_clGetMemObjectInfo)(output, CL_MEM_SIZE, sizeof(sz), &sz, NULL);
        if (debug >= 1) printf("copying %lu for output %p -> %p\n", sz, output, foutput);
        CL_CHECK((*p_clEnqueueReadBuffer)(command_queue, output, CL_TRUE, 0, sz, foutput, 0, NULL, NULL));
    } else {
        printf("CAUTION: model output is NULL, does it have no outputs?\n");
    }
}

// *********** CLQueuedKernel ***********

CLQueuedKernel::CLQueuedKernel(Thneed *lthneed,
                               cl_kernel _kernel,
                               cl_uint _work_dim,
                               const size_t *_global_work_size,
                               const size_t *_local_work_size) {
    thneed = lthneed;
    kernel = _kernel;
    work_dim = _work_dim;
    assert(work_dim <= 3);
    for (int i = 0; i < work_dim; i++) {
        global_work_size[i] = _global_work_size[i];
        local_work_size[i] = _local_work_size[i];
    }

    char _name[0x100];
    (*p_clGetKernelInfo)(kernel, CL_KERNEL_FUNCTION_NAME, sizeof(_name), _name, NULL);
    name = string(_name);
    (*p_clGetKernelInfo)(kernel, CL_KERNEL_NUM_ARGS, sizeof(num_args), &num_args, NULL);

    // get args
    for (int i = 0; i < num_args; i++) {
        char arg_name[0x100] = {0};
        (*p_clGetKernelArgInfo)(kernel, i, CL_KERNEL_ARG_NAME, sizeof(arg_name), arg_name, NULL);
        arg_names.push_back(string(arg_name));
        (*p_clGetKernelArgInfo)(kernel, i, CL_KERNEL_ARG_TYPE_NAME, sizeof(arg_name), arg_name, NULL);
        arg_types.push_back(string(arg_name));

        args.push_back(g_args[make_pair(kernel, i)]);
        args_size.push_back(g_args_size[make_pair(kernel, i)]);
    }

    // get program
    (*p_clGetKernelInfo)(kernel, CL_KERNEL_PROGRAM, sizeof(program), &program, NULL);
}

int CLQueuedKernel::get_arg_num(const char *search_arg_name) {
    for (int i = 0; i < num_args; i++) {
        if (arg_names[i] == search_arg_name) return i;
    }
    printf("failed to find %s in %s\n", search_arg_name, name.c_str());
    assert(false);
}

cl_int CLQueuedKernel::exec() {
    if (kernel == NULL) {
        kernel = (*p_clCreateKernel)(program, name.c_str(), NULL);
        arg_names.clear();
        arg_types.clear();

        for (int j = 0; j < num_args; j++) {
            char arg_name[0x100] = {0};
            (*p_clGetKernelArgInfo)(kernel, j, CL_KERNEL_ARG_NAME, sizeof(arg_name), arg_name, NULL);
            arg_names.push_back(string(arg_name));
            (*p_clGetKernelArgInfo)(kernel, j, CL_KERNEL_ARG_TYPE_NAME, sizeof(arg_name), arg_name, NULL);
            arg_types.push_back(string(arg_name));

            cl_int ret;
            if (args[j].size() != 0) {
                assert(args[j].size() == args_size[j]);
                ret = thneed_clSetKernelArg(kernel, j, args[j].size(), args[j].data());
            } else {
                ret = thneed_clSetKernelArg(kernel, j, args_size[j], NULL);
            }
            assert(ret == CL_SUCCESS);
        }
    }

    if (thneed->debug >= 1) {
        debug_print(thneed->debug >= 2);
    }

    return (*p_clEnqueueNDRangeKernel)(thneed->command_queue,
                                  kernel, work_dim, NULL, global_work_size, local_work_size, 0, NULL, NULL);
}

void CLQueuedKernel::debug_print(bool verbose) {
    printf("%p %56s -- ", kernel, name.c_str());
    for (int i = 0; i < work_dim; i++) {
        printf("%4zu ", global_work_size[i]);
    }
    printf(" -- ");
    for (int i = 0; i < work_dim; i++) {
        printf("%4zu ", local_work_size[i]);
    }
    printf("\n");

    if (verbose) {
        for (int i = 0; i < num_args; i++) {
            string arg = args[i];
            printf("  %s %s", arg_types[i].c_str(), arg_names[i].c_str());
            void *arg_value = (void*)arg.data();
            int arg_size = arg.size();
            if (arg_size == 0) {
                printf(" (size) %d", args_size[i]);
            } else if (arg_size == 1) {
                printf(" = %d", *((char*)arg_value));
            } else if (arg_size == 2) {
                printf(" = %d", *((short*)arg_value));
            } else if (arg_size == 4) {
                if (arg_types[i] == "float") {
                    printf(" = %f", *((float*)arg_value));
                } else {
                    printf(" = %d", *((int*)arg_value));
                }
            } else if (arg_size == 8) {
                cl_mem val = (cl_mem)(*((uintptr_t*)arg_value));
                printf(" = %p", val);
                if (val != NULL) {
                    cl_mem_object_type obj_type;
                    (*p_clGetMemObjectInfo)(val, CL_MEM_TYPE, sizeof(obj_type), &obj_type, NULL);
                    if (arg_types[i] == "image2d_t" || arg_types[i] == "image1d_t" || obj_type == CL_MEM_OBJECT_IMAGE2D) {
                        cl_image_format format;
                        size_t width, height, depth, array_size, row_pitch, slice_pitch;
                        cl_mem buf;
                        (*p_clGetImageInfo)(val, CL_IMAGE_FORMAT, sizeof(format), &format, NULL);
                        assert(format.image_channel_order == CL_RGBA);
                        assert(format.image_channel_data_type == CL_HALF_FLOAT || format.image_channel_data_type == CL_FLOAT);
                        (*p_clGetImageInfo)(val, CL_IMAGE_WIDTH, sizeof(width), &width, NULL);
                        (*p_clGetImageInfo)(val, CL_IMAGE_HEIGHT, sizeof(height), &height, NULL);
                        (*p_clGetImageInfo)(val, CL_IMAGE_ROW_PITCH, sizeof(row_pitch), &row_pitch, NULL);
                        (*p_clGetImageInfo)(val, CL_IMAGE_DEPTH, sizeof(depth), &depth, NULL);
                        (*p_clGetImageInfo)(val, CL_IMAGE_ARRAY_SIZE, sizeof(array_size), &array_size, NULL);
                        (*p_clGetImageInfo)(val, CL_IMAGE_SLICE_PITCH, sizeof(slice_pitch), &slice_pitch, NULL);
                        assert(depth == 0);
                        assert(array_size == 0);
                        assert(slice_pitch == 0);

                        (*p_clGetImageInfo)(val, CL_IMAGE_BUFFER, sizeof(buf), &buf, NULL);
                        size_t sz = 0;
                        if (buf != NULL) (*p_clGetMemObjectInfo)(buf, CL_MEM_SIZE, sizeof(sz), &sz, NULL);
                        printf(" image %zu x %zu rp %zu @ %p buffer %zu", width, height, row_pitch, buf, sz);
                    } else {
                        size_t sz;
                        (*p_clGetMemObjectInfo)(val, CL_MEM_SIZE, sizeof(sz), &sz, NULL);
                        printf(" buffer %zu", sz);
                    }
                }
            }
            printf("\n");
        }
    }
}

cl_int thneed_clSetKernelArg(cl_kernel kernel, cl_uint arg_index, size_t arg_size, const void *arg_value) {
    g_args_size[make_pair(kernel, arg_index)] = arg_size;
    if (arg_value != NULL) {
        g_args[make_pair(kernel, arg_index)] = string((char*)arg_value, arg_size);
    } else {
        g_args[make_pair(kernel, arg_index)] = string("");
    }
    cl_int ret = (*p_clSetKernelArg)(kernel, arg_index, arg_size, arg_value);
    return ret;
}

ThneedModel::ThneedModel(const std::string path, float *_output, size_t _output_size, int runtime, bool luse_tf8, cl_context context) {
    thneed = new Thneed(true, context);
    thneed->load(path.c_str());
    thneed->clexec();

    recorded = false;
    output = _output;
}

void* ThneedModel::getCLBuffer(const std::string name) {
    int index = -1;
    for (int i = 0; i < inputs.size(); i++) {
        if (name == inputs[i]->name) {
            index = i;
            break;
        }
    }

    if (thneed->input_clmem.size() >= inputs.size()) {
        return &thneed->input_clmem[inputs.size() - index - 1];
    } else {
        return nullptr;
    }
}

void ThneedModel::execute() {
    if (!recorded) {
        thneed->record = true;
        float *input_buffers[inputs.size()];
        for (int i = 0; i < inputs.size(); i++) {
            input_buffers[inputs.size() - i - 1] = inputs[i]->buffer;
        }

        thneed->copy_inputs(input_buffers);
        thneed->clexec();
        thneed->copy_output(output);
        thneed->stop();

        recorded = true;
    } else {
        float *input_buffers[inputs.size()];
        for (int i = 0; i < inputs.size(); i++) {
            input_buffers[inputs.size() - i - 1] = inputs[i]->buffer;
        }
        thneed->execute(input_buffers, output);
    }
}

std::string *pathString;
jfloat* outputs;
jint output_len;
ThneedModel *thneed;
jboolean inputsSet = false;

extern "C" {

    void JNICALL Java_ai_flow_android_vision_THNEEDModelRunner_createStdString(JNIEnv *env, jclass clazz, jstring javaString) {
        // Convert Java string to C++ string
        const char *cString = env->GetStringUTFChars(javaString, 0);
        pathString = new std::string(cString);

        // Release the C string
        env->ReleaseStringUTFChars(javaString, cString);
    }

    void JNICALL Java_ai_flow_android_vision_THNEEDModelRunner_getArray(JNIEnv *env, jobject obj, jint size) {
        // Allocate a float array of the given size
        outputs = new jfloat[size];
        output_len = size;
    }

    void JNICALL Java_ai_flow_android_vision_THNEEDModelRunner_initThneed(JNIEnv *env, jobject obj) {
        /*cl_int err;
        cl_uint numPlatforms;
        err = clGetPlatformIDs(0, NULL, &numPlatforms);
        std::vector<cl_platform_id> platforms(numPlatforms);
        err = clGetPlatformIDs(numPlatforms, platforms.data(), NULL);
        cl_uint numDevices;
        err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 0, NULL, &numDevices); // Change to CPU if no GPU is available
        std::vector<cl_device_id> devices(numDevices);
        err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, numDevices, devices.data(), NULL); // Change to CPU if no GPU is available
        cl_context context = clCreateContext(NULL, numDevices, devices.data(), NULL, NULL, &err);*/
        // this actually makes a context, so we will just pass NULL
        thneed = new ThneedModel(*pathString, outputs, output_len, 0, false, NULL);
    }

    JNIEXPORT jfloatArray JNICALL Java_ai_flow_android_vision_THNEEDModelRunner_executeModel(JNIEnv *env, jobject obj,
                                                               jfloatArray input_imgs,
                                                               jfloatArray big_input_imgs,
                                                               jfloatArray features_buffer,
                                                               jfloatArray desire,
                                                               jfloatArray traffic_convention,
                                                               jfloatArray nav_features,
                                                               jfloatArray nav_instructions) {
        // get sizes
        jsize input_imgs_len = env->GetArrayLength(input_imgs);
        jsize big_input_imgs_len = env->GetArrayLength(big_input_imgs);
        jsize features_buffer_len = env->GetArrayLength(features_buffer);
        jsize desire_len = env->GetArrayLength(desire);
        jsize traffic_convention_len = env->GetArrayLength(traffic_convention);
        jsize nav_features_len = env->GetArrayLength(nav_features);
        jsize nav_instructions_len = env->GetArrayLength(nav_instructions);

        // buffers
        jfloat *input_imgs_buf = env->GetFloatArrayElements(input_imgs, 0);
        jfloat *big_input_imgs_buf = env->GetFloatArrayElements(big_input_imgs, 0);
        jfloat *features_buffer_buf = env->GetFloatArrayElements(features_buffer, 0);
        jfloat *desire_buf = env->GetFloatArrayElements(desire, 0);
        jfloat *traffic_convention_buf = env->GetFloatArrayElements(traffic_convention, 0);
        jfloat *nav_features_buf = env->GetFloatArrayElements(nav_features, 0);
        jfloat *nav_instructions_buf = env->GetFloatArrayElements(nav_instructions, 0);

        if (inputsSet) {
            thneed->setInputBuffer("input_imgs", input_imgs_buf, input_imgs_len);
            thneed->setInputBuffer("big_input_imgs", big_input_imgs_buf, big_input_imgs_len);
            thneed->setInputBuffer("features_buffer", features_buffer_buf, features_buffer_len);
            thneed->setInputBuffer("desire", desire_buf, desire_len);
            thneed->setInputBuffer("traffic_convention", traffic_convention_buf, traffic_convention_len);
            thneed->setInputBuffer("nav_features", nav_features_buf, nav_features_len);
            thneed->setInputBuffer("nav_instructions", nav_instructions_buf, nav_instructions_len);
        } else {
            thneed->addInput("input_imgs", input_imgs_buf, input_imgs_len);
            thneed->addInput("big_input_imgs", big_input_imgs_buf, big_input_imgs_len);
            thneed->addInput("features_buffer", features_buffer_buf, features_buffer_len);
            thneed->addInput("desire", desire_buf, desire_len);
            thneed->addInput("traffic_convention", traffic_convention_buf, traffic_convention_len);
            thneed->addInput("nav_features", nav_features_buf, nav_features_len);
            thneed->addInput("nav_instructions", nav_instructions_buf, nav_instructions_len);
            inputsSet = true;
        }

        // ok execute model
        thneed->execute();

        // When done, release the memory
        env->ReleaseFloatArrayElements(input_imgs, input_imgs_buf, 0);
        env->ReleaseFloatArrayElements(big_input_imgs, big_input_imgs_buf, 0);
        env->ReleaseFloatArrayElements(features_buffer, features_buffer_buf, 0);
        env->ReleaseFloatArrayElements(desire, desire_buf, 0);
        env->ReleaseFloatArrayElements(traffic_convention, traffic_convention_buf, 0);
        env->ReleaseFloatArrayElements(nav_features, nav_features_buf, 0);
        env->ReleaseFloatArrayElements(nav_instructions, nav_instructions_buf, 0);

        // get the outputs
        jfloatArray result = env->NewFloatArray(output_len);
        env->SetFloatArrayRegion(result, 0, output_len, outputs);

        return result;
    }

} // extern "C"
