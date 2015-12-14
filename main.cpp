/*Copyright (c) 2015 Alex Hultman

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgement in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.*/

#include <iostream>
#include <cstdio>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
using namespace std;

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
using namespace boost::filesystem;
using namespace boost::program_options;
using namespace boost::algorithm;

vector<string> locate(string elf)
{
    FILE *pipe = popen(("locate " + elf).c_str(), "r");
    vector<string> lines;
    string line;
    for(int c; (c = fgetc(pipe)) != EOF; /*line += (char) c*/) {
        if (c == '\n')
        {
            lines.push_back(line);
            line.clear();
        }
        else {
            line += (char) c;
        }
    }
    pclose(pipe);
    return lines;
}

void depsolve(string elf, set<string> &files)
{
    files.insert(elf);
    FILE *pipe = popen(("ldd " + elf + " 2> /dev/null").c_str(), "r");
    string line;
    for(int c; (c = fgetc(pipe)) != EOF; line += (char) c)
        if(c == '\n')
        {
            size_t start;
            if((start = line.find('/')) != string::npos)
                files.insert(line.substr(start, line.find(' ', start) - start));
            line.clear();
        }
    pclose(pipe);
}

string resolveCommand(string command)
{
    FILE *pipe = popen(("which " + command + " 2> /dev/null").c_str(), "r");
    string line;
    for(int c; (c = fgetc(pipe)) != EOF && c != '\n'; line += (char) c);
    pclose(pipe);
    return line.length() ? line : command;
}

int main(int argc, char **argv)
{   
    int expose = -1;
    bool tar = false, essentials = false, opencl = false;
    string output, from;
    vector<string> cmd;
    try
    {
        options_description options("options"), hidden, all;
        options.add_options()("expose", value<int>(&expose)->value_name("<port>"), "Expose container port <port>")
                          ("from", value<string>(&from)->value_name("<image>")->default_value("scratch"), "Derive from image <image>")
                          ("tar", bool_switch(&tar), "Tar container instead of zip")
                          ("filename", value<string>(&output)->value_name("<filename>")->default_value("container"), "Output filename without extension")
                          ("essentials", bool_switch(&essentials), "Base container on busybox, a minimal <3mb image")
                          ("opencl", bool_switch(&opencl), "Include /etc/OpenCL/vendors and its resolved content")
                          ("help", "Show this help");

        hidden.add_options()("cmd", value<vector<string>>(&cmd));
        all.add(options).add(hidden);

        positional_options_description pd;
        pd.add("cmd", -1);

        variables_map vm;
        store(command_line_parser(argc, argv).options(all).positional(pd).run(), vm);
        notify(vm);

        if(argc == 1 || vm.count("help"))
        {
            cout << "Usage: containerize [options] ELF args.." << endl << endl;
            cout << options << endl;
            return 0;
        }
    }
    catch(exception &e)
    {
        cerr << "error: " << e.what() << "\n";
        return 1;
    }

    set<string> files;
    string currentPath = current_path().string();
    for(recursive_directory_iterator it(currentPath), end; it != end; it++)
        if(is_regular_file(it->status()))
            depsolve(it->path().string().substr(currentPath.length() + 1), files);

    if (opencl) {
        for(recursive_directory_iterator it("/etc/OpenCL/vendors"), end; it != end; it++)
            if(is_regular_file(it->status()))
            {
                files.insert(it->path().string());
                ifstream fin(it->path().string());
                string driver;
                fin >> driver;

                for (string &x : locate(driver)) {
                    depsolve(x, files);
                }

                // Ugly haxx for nvidia driver
                if (driver.substr(0, 16) == "libnvidia-opencl") {
                    for (string &x : locate("libnvidia-compiler.so")) {
                        files.insert(x);
                    }
                }

                // Ugly haxx for intel driver
                if (driver.substr(0, 10) == "/opt/intel") {
                    for(recursive_directory_iterator it(path(driver).parent_path()), end; it != end; it++)
                        if(is_regular_file(it->status())) {
                            files.insert(it->path().string());
                            //depsolve(it->path().string(), files);
                        }
                }

                depsolve(driver, files);
            }
    }

    cmd[0] = resolveCommand(cmd[0]);

    // Strip current path base
    if (!cmd[0].compare(0, currentPath.length(), currentPath)) {
        cmd[0] = cmd[0].substr(currentPath.length());
    }

    depsolve(cmd[0], files);
    ofstream dockerfile("Dockerfile");
    dockerfile << "FROM " << (essentials ? "busybox" : from) << endl
        << "ADD . /" << endl << "CMD [\"" << join(cmd, "\", \"") << "\"]";
    if(expose != -1)
        dockerfile << endl << "EXPOSE " << expose << endl;
    dockerfile.close();

    system(((tar ? "tar " : "zip ") + output + " " + join(files, " ") + " Dockerfile > /dev/null && rm Dockerfile").c_str());
}
