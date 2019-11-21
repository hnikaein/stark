/**
 * @author Hassan Nikaein
 */

#include <string>

#ifndef TIME_PROFILE_H
#define TIME_PROFILE_H

void add_time_c(const std::string &caller_name);

std::string get_times_str_c(const std::string &caller_name, bool free_space = true);

long last_time_c(const std::string &caller_name);

void erase_times_c(const std::string &caller_name);

#define add_time() add_time_c(string(__func__))
#define atomic_add_time() add_time_c(string(__func__) + to_string(call_id))
#define get_times_str(x) get_times_str_c(string(__func__),x).c_str()
#define atomic_get_times_str(x) get_times_str_c(string(__func__) + to_string(call_id), x).c_str()
#define last_time() last_time_c(string(__func__))
#define erase_times() erase_times_c(string(__func__))

#endif //TIME_PROFILE_H
