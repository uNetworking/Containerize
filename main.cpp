#include <iostream>
#include <cstdio>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
using namespace std;

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
using namespace boost::filesystem;
using namespace boost::program_options;

vector<string> depsolve(string elf)
{
    vector<string> deps;
    FILE *pipe = popen(("ldd " + elf + " 2> /dev/null").c_str(), "r");
    string line;
    for(int c; (c = fgetc(pipe)) != EOF; line += (char) c)
        if(c == '\n')
        {
            size_t start;
            if((start = line.find('/')) != string::npos)
                deps.push_back(line.substr(start, line.find(' ', start) - start));
            line.clear();
        }
    pclose(pipe);
    return deps;
}

void addFileAndDeps(string &fileName, string &commandLine)
{
    commandLine += " ";
    commandLine += fileName;

    for(string dep : depsolve(fileName))
    {
        commandLine += " ";
        commandLine += dep;
    }
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
    string from = "scratch";
    bool tar = 0;
    string output;
    vector<string> cmd;
    try
    {
        options_description options("options"), hidden, all;
        options.add_options()("expose", value<int>(&expose)->value_name("<port>"), "Expose container port <port>")
                          ("from", value<string>(&from)->value_name("<image>"), "Derive from image <image>, default is scratch")
                          ("tar", "Tar container instead of zip")
                          ("filename", value<string>(&output)->value_name("<file name>"), "Output filename without extension")
                          ("help", "Show this help");

        hidden.add_options()("cmd", value<vector<string>>());
        all.add(options).add(hidden);

        positional_options_description pd;
        pd.add("cmd", -1);

        variables_map vm;
        store(command_line_parser(argc, argv).options(all).positional(pd).run(), vm);
        notify(vm);

        if(!vm.size() || vm.count("help"))
        {
            cout << "Usage: containerize [options] command line" << endl << endl;
            cout << options << endl;
            return 0;
        }

        tar = vm.count("tar");
        cmd = vm["cmd"].as<vector<string>>();
    }
    catch(exception &e)
    {
        cerr << "error: " << e.what() << "\n";
        return 1;
    }

    //Lägg till alla filer och dess beroenden
    if(!output.length())
        output = "container";
    string commandLine = "zip " + output;
    string currentPath = boost::filesystem::current_path().string();
    for(recursive_directory_iterator it(currentPath), end; it != end; it++)
        if(is_regular_file(it->status()))
        {
            string fileName = it->path().string().substr(currentPath.length() + 1);
            addFileAndDeps(fileName, commandLine);
        }


    ofstream dockerfile("Dockerfile");
    dockerfile << "FROM " << from << endl;
    dockerfile << "ADD . /" << endl;
    dockerfile << "CMD [";

    int commands = 1;
    int i = 0;
    for(string c : cmd)
    {
        if(commands-- > 0)
        {
            c = resolveCommand(c);
            addFileAndDeps(c, commandLine);
            commandLine += " ";
            commandLine += c;
        }

        if(i++ > 0)
            dockerfile << ", ";
        dockerfile << "\"" << c << "\"";
    }

    dockerfile << "]" << endl;


    if(expose != -1)
        dockerfile << "EXPOSE " << expose << endl;
    dockerfile.close();

    commandLine += " Dockerfile > /dev/null";
    system(commandLine.c_str());
    system("rm Dockerfile");
}
