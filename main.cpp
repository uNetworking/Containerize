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
    bool tar = 0, essentials = 0;
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

    cmd[0] = resolveCommand(cmd[0]);
    depsolve(cmd[0], files);
    ofstream dockerfile("Dockerfile");
    dockerfile << "FROM " << (essentials ? "busybox" : from) << endl
        << "ADD . /" << endl << "CMD [\"" << join(cmd, "\", \"") << "\"]";
    if(expose != -1)
        dockerfile << "EXPOSE " << expose << endl;
    dockerfile.close();

    system(("zip " + output + " " + join(files, " ") + " Dockerfile > /dev/null && rm Dockerfile").c_str());
}
