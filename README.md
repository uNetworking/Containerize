## Containerize
Tool for creating minimal Docker containers. License is zlib/libpng.  

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
  --help                             Show this help  
  ```
  
  Containerize focuses on a Linux executable (ELF) and collects its dependencies (shared objects), packages them with the ELF and writes a Dockerfile. Basic features like exposing a network port and deriving the image from others are supported.  
    
  If you run your local app like: **./my_app arg_1 arg_2** then to create a container of this app you simply add **containerize** as a prefix, like so: **containerize ./my_app arg_1 arg_2**. This will create a container. To expose a network port you have to add the --expose argument accordingly.
