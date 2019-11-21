/**
 * @author Hassan Nikaein
 */

#include "time_profile.h"
#include <chrono>
#include <vector>
#include <map>

using namespace std;

auto times = map<string, vector<chrono::milliseconds>>();

void add_time_c(const string &caller_name) {
    using namespace chrono;
    if (!times.count(caller_name))
        times[caller_name] = vector<chrono::milliseconds>();
    times[caller_name].push_back(duration_cast<milliseconds>(
            system_clock::now().time_since_epoch()
    ));
}

string get_times_str_c(const string &caller_name, bool free_space) {
    string res;
    for (int i = 1; i < times[caller_name].size(); i++)
        res += to_string(times[caller_name][i].count() - times[caller_name][i - 1].count()) + " ";
    if (free_space)
        times.erase(caller_name);
    return res;
}

long last_time_c(const string &caller_name) {
    unsigned long siz = times[caller_name].size();
    return times[caller_name][siz - 1].count() - times[caller_name][siz - 2].count();
}

void erase_times_c(const string &caller_name) {
    times.erase(caller_name);
}