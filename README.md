## Containerize
Tool for creating minimal Docker containers. License is zlib/libpng.  

It does work automatically in very simple cases but might need manual intervention when executables have runtime dependencies not detected by LDD. This is the case with OpenCL driver dependencies which needs custom ugly haxxes.  
--opencl flag is used to collect the minimal OpenCL driver, which currently works with NVIDIA and Intel OpenCL drivers.

The point is, instead of using some pre-made 700mb large Docker image with crap you don't need, this tool helps you deploy ELF binaries with minimal overhead (20mb or whatever). The point of Docker is to skip the crap you previously needed when deploying complete OS:es in virtual machines.

### Usage
```
Usage: containerize [options] ELF args..

options:
  --expose <port>                    Expose container port <port>
  --from <image> (=scratch)          Derive from image <image>
  --tar                              Tar container instead of zip
  --filename <filename> (=container) Output filename without extension
  --essentials                       Base container on busybox, a minimal <3mb 
                                     image
  --opencl                           Include /etc/OpenCL/vendors and its 
                                     resolved content
  --help                             Show this help  
  ```
  
  Containerize focuses on a Linux executable (ELF) and collects its dependencies (shared objects), packages them with the ELF and writes a Dockerfile. Basic features like exposing a network port and deriving the image from others are supported.  
    
  If you run your local app like: **./my_app arg_1 arg_2** then to create a container of this app you simply add **containerize** as a prefix, like so: **containerize ./my_app arg_1 arg_2**. This will create a container. To expose a network port you have to add the --expose argument accordingly.
