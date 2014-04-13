#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <EventType.hpp>

using namespace std;
using namespace boost;


string root_dir = "/projects/wildlife/feature_files/";
string output_file = "svm.dat";
vector<string> positive_files;
vector<string> negative_files;
vector<EventType*> *event_types;

int main(int argc, char **argv) {
    namespace po = program_options;
    namespace fs = filesystem;
    namespace sys = system;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Show help menu")
        ("root,r", po::value<string>(), "Root feature directory")
        ("positive,p", po::value<string>(), "Tags for positive features")
        ("negative,n", po::value<string>(), "Tags for negative features")
        ("output,o", po::value<string>(), "Filename for SVM features")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help") || !vm.count("root") || !vm.count("positive")) {
        cout << desc << endl;
        return 1;
    }

    root_dir = vm["root"].as<string>();
    positive_files.push_back(vm["positive"].as<string>());

    if (vm.count("negative")) {
        negative_files.push_back(vm["negative"].as<string>());
    }

    if (vm.count("output")) {
        output_file = vm["output"].as<string>();
    }

    cout << "Root       : '" << root_dir << "'" << endl;
    cout << "Positive   : " << endl;
    for(int i=0; i < positive_files.size(); i++) {
        cout << "\t" << positive_files[i] << endl;
    }
    cout << "Negative   : " << endl;
    for(int i=0; i < negative_files.size(); i++) {
        cout << "\t" << negative_files[i] << endl;
    }
    cout << "Output     : '" << output_file << "'" << endl;
}
